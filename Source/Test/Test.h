#pragma once

#include "../Include/NTAssassin/CRT.h"
#include "../Include/NTAssassin/Extension.h"
#include "../Include/NTAssassin/NTDef.h"

EXTERN_C_START

/* Tests */
BOOL Test_new_delete();
BOOL Test_Memory_Allocate();
BOOL Test_String();

/* Samples */
BOOL Sample_ListFile();
BOOL Sample_QueryStorageProperty();
BOOL Sample_PrintFirmwareTable();

/* Internals */

/* Set CRT error mode as Ntdll CRT */
VOID SetNtdllCRTErrorMode();
VOID RevertCRTErrorMode();

/*
 * Str_CchPrint(A/W)
 *
 * Return == 0: Error or no data
 * Return < BufferCount: Success
 * Return >= BufferCount: Truncated, returns required size in character, not including null-terminator
 */

_Success_(
    return > 0 && return < BufferCount
)
ULONG Str_CchPrintfA(
    _Out_writes_opt_(BufferCount) _Always_(_Post_z_) PSTR CONST Buffer,
    _In_ SIZE_T CONST BufferCount,
    _In_z_ _Printf_format_string_ PCSTR Format, ...);

_Success_(
    return > 0 && return < BufferCount
)
ULONG Str_CchPrintfW(
    _Out_writes_opt_(BufferCount) _Always_(_Post_z_) PWSTR CONST Buffer,
    _In_ SIZE_T CONST BufferCount,
    _In_z_ _Printf_format_string_ PCWSTR Format, ...);

VOID PrintF(_In_z_ _Printf_format_string_ PCSTR Format, ...);
VOID PrintTitle(_In_z_ PCSTR Name, _In_z_ PCSTR Description);

/* NT function wrapper */

_Success_(return != FALSE)
BOOL NTAPI NT_InitializePathObjectEx(
    _In_z_ PCWSTR Path,
    _In_opt_ HANDLE RootDirectory,
    _In_ ULONG Attributes,
    _In_opt_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Out_ POBJECT_ATTRIBUTES Object,
    _Out_ PUNICODE_STRING ObjectName,
    _Out_opt_ PRTL_RELATIVE_NAME_U RelativeName);

_Success_(return != FALSE)
BOOL NTAPI NT_InitializePathObject(
    _In_z_ PCWSTR Path,
    _In_ BOOL InheritHandle,
    _Out_ POBJECT_ATTRIBUTES Object,
    _Out_ PUNICODE_STRING ObjectName,
    _Out_opt_ PRTL_RELATIVE_NAME_U RelativeName);

HANDLE IO_OpenDevice(_In_ PUNICODE_STRING DeviceName, _In_ ACCESS_MASK DesiredAccess);

_Ret_maybenull_
PSYSTEM_FIRMWARE_TABLE_INFORMATION NT_GetFirmwareTable(
    ULONG ProviderSignature,
    ULONG TableID,
    SYSTEM_FIRMWARE_TABLE_ACTION Action);

EXTERN_C_END
