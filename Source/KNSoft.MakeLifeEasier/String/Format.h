#pragma once

#include "../MakeLifeEasier.h"

#define GUID_TO_STRING_FORMAT_LOWER "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"
#define GUID_TO_STRING_FORMAT_UPPER "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"

EXTERN_C_START

#pragma region GUID

FORCEINLINE
_Success_(return > 0)
ULONG
Str_FromGUIDLowerW(
    _Out_writes_(BufferCount) _Always_(_Post_z_) PWSTR Buffer,
    _In_ ULONG BufferCount,
    _In_ GUID* Guid)
{
    return StrSafe_CchPrintfW(Buffer, BufferCount, _A2W(GUID_TO_STRING_FORMAT_LOWER),
                              Guid->Data1, Guid->Data2, Guid->Data3,
                              Guid->Data4[0], Guid->Data4[1], Guid->Data4[2], Guid->Data4[3],
                              Guid->Data4[4], Guid->Data4[5], Guid->Data4[6], Guid->Data4[7]);
}

FORCEINLINE
_Success_(return > 0)
ULONG
Str_FromGUIDLowerA(
    _Out_writes_(BufferCount) _Always_(_Post_z_) PSTR Buffer,
    _In_ ULONG BufferCount,
    _In_ GUID* Guid)
{
    return StrSafe_CchPrintfA(Buffer, BufferCount, GUID_TO_STRING_FORMAT_LOWER,
                              Guid->Data1, Guid->Data2, Guid->Data3,
                              Guid->Data4[0], Guid->Data4[1], Guid->Data4[2], Guid->Data4[3],
                              Guid->Data4[4], Guid->Data4[5], Guid->Data4[6], Guid->Data4[7]);
}

FORCEINLINE
_Success_(return > 0)
ULONG
Str_FromGUIDUpperW(
    _Out_writes_(BufferCount) _Always_(_Post_z_) PWSTR Buffer,
    _In_ ULONG BufferCount,
    _In_ GUID* Guid)
{
    return StrSafe_CchPrintfW(Buffer, BufferCount, _A2W(GUID_TO_STRING_FORMAT_UPPER),
                              Guid->Data1, Guid->Data2, Guid->Data3,
                              Guid->Data4[0], Guid->Data4[1], Guid->Data4[2], Guid->Data4[3],
                              Guid->Data4[4], Guid->Data4[5], Guid->Data4[6], Guid->Data4[7]);
}

FORCEINLINE
_Success_(return > 0)
ULONG
Str_FromGUIDUpperA(
    _Out_writes_(BufferCount) _Always_(_Post_z_) PSTR Buffer,
    _In_ ULONG BufferCount,
    _In_ GUID* Guid)
{
    return StrSafe_CchPrintfA(Buffer, BufferCount, GUID_TO_STRING_FORMAT_UPPER,
                              Guid->Data1, Guid->Data2, Guid->Data3,
                              Guid->Data4[0], Guid->Data4[1], Guid->Data4[2], Guid->Data4[3],
                              Guid->Data4[4], Guid->Data4[5], Guid->Data4[6], Guid->Data4[7]);
}

#pragma endregion

EXTERN_C_END
