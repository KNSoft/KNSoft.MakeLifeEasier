#pragma once

#include "../MakeLifeEasier.h"

#include "../PE/Util.h"

EXTERN_C_START

#pragma region Memory R/W

MLE_API
NTSTATUS
NTAPI
PS_32Read64Memory(
    _In_ HANDLE ProcessHandle,
    _In_ VOID* POINTER_64 BaseAddress,
    _Out_writes_bytes_(NumberOfBytesToRead) PVOID Buffer,
    _In_ ULONGLONG NumberOfBytesToRead,
    _Out_opt_ PULONGLONG NumberOfBytesRead);

FORCEINLINE
NTSTATUS
PS_RWStatus(
    _In_ NTSTATUS Status)
{
    return Status != STATUS_PARTIAL_COPY ? Status : STATUS_DATA_ERROR;
}

MLE_API
NTSTATUS
NTAPI
PS_DuplicateUnicodeString64(
    _In_ HANDLE ProcessHandle,
    _In_ PUNICODE_STRING64 Src,
    _Out_ PUNICODE_STRING* Dst);

MLE_API
NTSTATUS
NTAPI
PS_DuplicateUnicodeString32(
    _In_ HANDLE ProcessHandle,
    _In_ PUNICODE_STRING32 Src,
    _Out_ PUNICODE_STRING* Dst);

FORCEINLINE
_Success_(return != FALSE)
LOGICAL
PS_FreeUnicodeString(
    __drv_freesMem(Mem) _Frees_ptr_ _Post_invalid_ PUNICODE_STRING String)
{
    return NT_FreeStringW(String);
}

#pragma endregion

/* Require PROCESS_QUERY_LIMITED_INFORMATION */
MLE_API
NTSTATUS
NTAPI
PS_GetMachineType(
    _In_ HANDLE ProcessHandle,
    _Out_ PUSHORT MachineType);

FORCEINLINE
NTSTATUS
NTAPI
PS_GetMachineBits(
    _In_ HANDLE ProcessHandle,
    _Out_ PUSHORT MachineBits)
{
    NTSTATUS Status;
    USHORT MachineType;
    USHORT Bits;

    Status = PS_GetMachineType(ProcessHandle, &MachineType);
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

#pragma region Enumerate Modules

MLE_API
NTSTATUS
NTAPI
PS_EnumerateModules64(
    _In_ HANDLE ProcessHandle,
    _In_ PLDR_ENUM_CALLBACK64 EnumProc,
    _In_ PVOID Context);

MLE_API
NTSTATUS
NTAPI
PS_EnumerateModules32(
    _In_ HANDLE ProcessHandle,
    _In_ PLDR_ENUM_CALLBACK32 EnumProc,
    _In_ PVOID Context);

MLE_API
NTSTATUS
NTAPI
PS_GetRemoteModuleEntryByAddress64(
    _In_ HANDLE ProcessHandle,
    _In_ VOID* POINTER_64 Address,
    _Out_ PLDR_DATA_TABLE_ENTRY64 ModuleEntry);

MLE_API
NTSTATUS
NTAPI
PS_GetRemoteModuleEntryByAddress32(
    _In_ HANDLE ProcessHandle,
    _In_ VOID* POINTER_32 Address,
    _Out_ PLDR_DATA_TABLE_ENTRY32 ModuleEntry);

MLE_API
HRESULT
NTAPI
PS_GetRemoteAddressName(
    _In_ HANDLE ProcessHandle,
    _In_ ULONGLONG Address,
    _Outptr_opt_ PUNICODE_STRING* ModulePath,
    _Outptr_opt_result_maybenull_ PUNICODE_STRING* SymbolName,
    _Out_opt_ _When_(SymbolName == NULL, _Null_) PULONGLONG SymbolDisplacement);

#pragma endregion

EXTERN_C_END
