#pragma once

#include "../MakeLifeEasier.h"

#include "../NT/Object.h"

EXTERN_C_START

W32ERROR
NTAPI
PS_CreateProcess(
    _In_opt_ HANDLE TokenHandle,
    _In_opt_ PCWSTR ApplicationName,
    _Inout_opt_ PWSTR CommandLine,
    _In_ LOGICAL InheritHandles,
    _In_opt_ PCWSTR CurrentDirectory,
    _In_opt_ LPSTARTUPINFOW StartupInfo,
    _Out_ LPPROCESS_INFORMATION ProcessInformation);

FORCEINLINE
NTSTATUS
PS_CreateThread(
    _In_ HANDLE ProcessHandle,
    _In_ LOGICAL CreateSuspended,
    _In_ PUSER_THREAD_START_ROUTINE StartAddress,
    _In_opt_ PVOID Parameter,
    _Out_opt_ PHANDLE ThreadHandle,
    _Out_opt_ PULONG ThreadId)
{
    NTSTATUS Status;
    CLIENT_ID ClientId, *pClientId;

    pClientId = ThreadId == NULL ? NULL : &ClientId;
    Status = RtlCreateUserThread(ProcessHandle,
                                 NULL,
                                 !!CreateSuspended,
                                 0,
                                 0,
                                 0,
                                 StartAddress,
                                 Parameter,
                                 ThreadHandle,
                                 pClientId);
    if (NT_SUCCESS(Status) && ThreadId != NULL)
    {
        *ThreadId = (ULONG)(ULONG_PTR)ClientId.UniqueThread;
    }
    return Status;
}

FORCEINLINE
NTSTATUS
PS_OpenProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ProcessId)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    CLIENT_ID ClientId;

    NT_InitEmptyObject(&ObjectAttributes);
    ClientId.UniqueProcess = (HANDLE)(ULONG_PTR)ProcessId;
    ClientId.UniqueThread = 0;

    return NtOpenProcess(ProcessHandle, DesiredAccess, &ObjectAttributes, &ClientId);
}

FORCEINLINE
NTSTATUS
PS_OpenThread(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ThreadId)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    CLIENT_ID ClientId;

    NT_InitEmptyObject(&ObjectAttributes);
    ClientId.UniqueProcess = 0;
    ClientId.UniqueThread = (HANDLE)(ULONG_PTR)ThreadId;

    return NtOpenThread(ThreadHandle, DesiredAccess, &ObjectAttributes, &ClientId);
}

FORCEINLINE
NTSTATUS
PS_GetThreadExitCode(
    _In_ HANDLE ThreadHandle,
    _Out_ PULONG ExitCode)
{
    NTSTATUS Status;
    THREAD_BASIC_INFORMATION Info;

    Status = NtQueryInformationThread(ThreadHandle, ThreadBasicInformation, &Info, sizeof(Info), NULL);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    *ExitCode = Info.ExitStatus;
    return STATUS_SUCCESS;
}

EXTERN_C_END
