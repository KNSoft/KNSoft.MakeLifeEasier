#include "../MakeLifeEasier.inl"

#pragma region Heap

_Must_inspect_result_
__drv_allocatesMem(Mem)
NTSTATUS
NTAPI
Mem_Alloc(
    _Out_ _At_(*BaseAddress,
               _Readable_bytes_(Size)
               _Writable_bytes_(Size)
               _Post_readable_byte_size_(Size)) PVOID* BaseAddress,
    _In_ SIZE_T Size)
{
    PVOID Base = RtlAllocateHeap(NtGetProcessHeap(), 0, Size);
    if (Base == NULL)
    {
        return STATUS_NO_MEMORY;
    } else
    {
        *BaseAddress = Base;
    }
    return STATUS_SUCCESS;
}

LOGICAL
NTAPI
Mem_Free(
    __drv_freesMem(Mem) _Frees_ptr_ PVOID BaseAddress)
{
    return RtlFreeHeap(NtGetProcessHeap(), 0, BaseAddress);
}

#pragma endregion

#pragma region Page

_Must_inspect_result_
__drv_allocatesMem(Mem)
NTSTATUS
NTAPI
Mem_AllocPage(
    _Out_ _At_(*BaseAddress,
               _Readable_bytes_(Size)
               _Writable_bytes_(Size)
               _Post_readable_byte_size_(Size)) PVOID* BaseAddress,
    _In_ SIZE_T Size,
    _In_ ULONG Protect)
{
    PVOID Base = NULL;
    NTSTATUS Status;

    Status = NtAllocateVirtualMemory(NtCurrentProcess(), &Base, 0, &Size, MEM_COMMIT | MEM_RESERVE, Protect);
    if (NT_SUCCESS(Status))
    {
        *BaseAddress = Base;
    }
    return Status;
}

NTSTATUS
NTAPI
Mem_ProtectPage(
    _In_ PVOID BaseAddress,
    _In_ SIZE_T Size,
    _In_ ULONG Protect,
    _Out_ PULONG OldProtect)
{
    return NtProtectVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, Protect, OldProtect);
}

NTSTATUS
NTAPI
Mem_FreePage(
    __drv_freesMem(Mem) _Frees_ptr_ _In_ PVOID BaseAddress)
{
    SIZE_T Size = 0;

    return NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
}

#pragma endregion

_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
__drv_allocatesMem(Mem)
PVOID
WINAPIV
Mem_CombineStructEx(
    _In_ ULONG GroupCount,
    _In_ ULONG Size,
    ...)
{
    va_list args;
    SIZE_T i, u, uStructCount = 0;
    NTSTATUS Status;
    PVOID pBuffer, pDst, pSrc;

    va_start(args, Size);
    for (i = 0; i < GroupCount; i++)
    {
        pSrc = va_arg(args, PVOID);
        u = va_arg(args, ULONG);
        if (pSrc)
        {
            uStructCount += u;
        }
    }
    Status = Mem_Alloc(&pBuffer, uStructCount * Size);
    if (!NT_SUCCESS(Status))
    {
        return NULL;
    }

    pDst = pBuffer;
    va_start(args, Size);
    for (i = 0; i < GroupCount; i++)
    {
        pSrc = va_arg(args, PVOID);
        uStructCount = va_arg(args, ULONG);
        if (pSrc)
        {
            memcpy(pDst, pSrc, uStructCount * Size);
            pDst = Add2Ptr(pDst, uStructCount * Size);
        }
    }
    va_end(args);

    return pBuffer;
}
