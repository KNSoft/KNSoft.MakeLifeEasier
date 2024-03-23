#pragma once

#include "../NDK/NT/MinDef.h"

EXTERN_C_START

#pragma region String

NTA_API
_Success_(
    return > 0 && return < BufferCount
)
ULONG WINAPIV String_CchVPrintfA(
    _Out_writes_opt_(BufferCount) _Always_(_Post_z_) PSTR CONST Buffer,
    _In_ SIZE_T CONST BufferCount,
    _In_z_ _Printf_format_string_ PCSTR Format,
    va_list ArgList);

NTA_API
_Success_(
    return > 0 && return < BufferCount
)
ULONG WINAPIV String_CchVPrintfW(
    _Out_writes_opt_(BufferCount) _Always_(_Post_z_) PWSTR CONST Buffer,
    _In_ SIZE_T CONST BufferCount,
    _In_z_ _Printf_format_string_ PCWSTR Format,
    va_list ArgList);

NTA_API
_Success_(
    return > 0 && return < BufferCount
)
ULONG String_CchPrintfA(
    _Out_writes_opt_(BufferCount) _Always_(_Post_z_) PSTR CONST Buffer,
    _In_ SIZE_T CONST BufferCount,
    _In_z_ _Printf_format_string_ PCSTR Format, ...);

NTA_API
_Success_(
    return > 0 && return < BufferCount
)
ULONG String_CchPrintfW(
    _Out_writes_opt_(BufferCount) _Always_(_Post_z_) PWSTR CONST Buffer,
    _In_ SIZE_T CONST BufferCount,
    _In_z_ _Printf_format_string_ PCWSTR Format, ...);

#pragma endregion

EXTERN_C_END
