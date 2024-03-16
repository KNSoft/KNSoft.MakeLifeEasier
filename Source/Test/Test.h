#pragma once

#include "../Include/NTAssassin/CRT.h"
#include "../Include/NTAssassin/NTAssassin.h"

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

EXTERN_C_END
