#pragma once

#include "../../MakeLifeEasier.h"

#include "../../PE/Util.h"

EXTERN_C_START

#pragma region Memory R/W

MLE_API
NTSTATUS
NTAPI
PS_Remote32Read64Memory(
    _In_ HANDLE ProcessHandle,
    _In_ VOID* POINTER_64 BaseAddress,
    _Out_writes_bytes_(NumberOfBytesToRead) PVOID Buffer,
    _In_ ULONGLONG NumberOfBytesToRead,
    _Out_opt_ PULONGLONG NumberOfBytesRead);

/* Use NT_FreeStringW to free Dst */
MLE_API
NTSTATUS
NTAPI
PS_RemoteDuplicateUnicodeString64(
    _In_ HANDLE ProcessHandle,
    _In_ PUNICODE_STRING64 Src,
    _Out_ PUNICODE_STRING* Dst);

/* Use NT_FreeStringW to free Dst */
MLE_API
NTSTATUS
NTAPI
PS_RemoteDuplicateUnicodeString32(
    _In_ HANDLE ProcessHandle,
    _In_ PUNICODE_STRING32 Src,
    _Out_ PUNICODE_STRING* Dst);

#pragma endregion

/* Require PROCESS_QUERY_LIMITED_INFORMATION */
MLE_API
NTSTATUS
NTAPI
PS_RemoteGetMachineType(
    _In_ HANDLE ProcessHandle,
    _Out_ PUSHORT MachineType);

FORCEINLINE
NTSTATUS
NTAPI
PS_RemoteGetMachineBits(
    _In_ HANDLE ProcessHandle,
    _Out_ PUSHORT MachineBits)
{
    NTSTATUS Status;
    USHORT MachineType;
    USHORT Bits;

    Status = PS_RemoteGetMachineType(ProcessHandle, &MachineType);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Bits = PE_GetMachineBits(MachineType);
    if (Bits == 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    *MachineBits = Bits;
    return STATUS_SUCCESS;
}

FORCEINLINE
NTSTATUS
PS_RemoteGetWowPeb(
    _In_ HANDLE ProcessHandle,
    _Out_ PVOID* WowPeb)
{
    return NtQueryInformationProcess(ProcessHandle, ProcessWow64Information, WowPeb, sizeof(*WowPeb), NULL);
}

FORCEINLINE
NTSTATUS
PS_RemoteIsWow(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN IsWow)
{
    NTSTATUS Status;
    PVOID WowPeb;

    Status = PS_RemoteGetWowPeb(ProcessHandle, &WowPeb);
    if (NT_SUCCESS(Status))
    {
        *IsWow = WowPeb != NULL;
    }
    return Status;
}

#pragma region Enumerate Modules

MLE_API
NTSTATUS
NTAPI
PS_RemoteEnumerateModules64(
    _In_ HANDLE ProcessHandle,
    _In_ PLDR_ENUM_CALLBACK64 EnumProc,
    _In_ PVOID Context);

MLE_API
NTSTATUS
NTAPI
PS_RemoteEnumerateModules32(
    _In_ HANDLE ProcessHandle,
    _In_ PLDR_ENUM_CALLBACK32 EnumProc,
    _In_ PVOID Context);

MLE_API
NTSTATUS
NTAPI
PS_RemoteGetModuleEntryByAddress64(
    _In_ HANDLE ProcessHandle,
    _In_ VOID* POINTER_64 Address,
    _Out_ PLDR_DATA_TABLE_ENTRY64 ModuleEntry);

MLE_API
NTSTATUS
NTAPI
PS_RemoteGetModuleEntryByAddress32(
    _In_ HANDLE ProcessHandle,
    _In_ VOID* POINTER_32 Address,
    _Out_ PLDR_DATA_TABLE_ENTRY32 ModuleEntry);

MLE_API
NTSTATUS
NTAPI
PS_RemoteGetAddressName(
    _In_ HANDLE ProcessHandle,
    _In_ ULONGLONG Address,
    _Outptr_opt_ PUNICODE_STRING* ModulePath,
    _Outptr_opt_result_maybenull_ PUNICODE_STRING* SymbolName,
    _Out_opt_ _When_(SymbolName == NULL, _Null_) PULONGLONG SymbolDisplacement);

#pragma endregion

EXTERN_C_END
