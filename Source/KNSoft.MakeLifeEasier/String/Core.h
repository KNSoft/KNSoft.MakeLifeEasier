﻿#pragma once

#include <KNSoft/NDK/Package/StrSafe.h>
#include <KNSoft/NDK/NDK.h>

#pragma region CRT Alias

FORCEINLINE
SIZE_T
Str_LenW(
    _In_ PCWSTR String)
{
    return wcslen(String);
}

FORCEINLINE
SIZE_T
Str_LenA(
    _In_ PCSTR String)
{
    return strlen(String);
}

FORCEINLINE
INT
Str_CmpA(
    _In_ PCSTR String1,
    _In_ PCSTR String2)
{
    return strcmp(String1, String2);
}

FORCEINLINE
INT
Str_CmpW(
    _In_ PCWSTR String1,
    _In_ PCWSTR String2)
{
    return wcscmp(String1, String2);
}

FORCEINLINE
PCWSTR
Str_StrW(
    _In_ PCWSTR String,
    _In_ PCWSTR SubString)
{
    return wcsstr(String, SubString);
}

FORCEINLINE
PCSTR
Str_StrA(
    _In_ PCSTR String,
    _In_ PCSTR SubString)
{
    return strstr(String, SubString);
}

#pragma endregion

#pragma region Basic Operations

/* Str_Size */

FORCEINLINE
SIZE_T
Str_SizeW(
    _In_ PCWSTR String)
{
    return Str_LenW(String) * sizeof(WCHAR);
}

FORCEINLINE
SIZE_T
Str_SizeA(
    _In_ PCSTR String)
{
    return Str_LenA(String) * sizeof(CHAR);
}

/* Str_Equal */

FORCEINLINE
LOGICAL
Str_EqualA(
    _In_ PCSTR String1,
    _In_ PCSTR String2)
{
    return Str_CmpA(String1, String2) == 0;
}

FORCEINLINE
LOGICAL
Str_EqualW(
    _In_ PCWSTR String1,
    _In_ PCWSTR String2)
{
    return Str_CmpW(String1, String2) == 0;
}

#pragma endregion

#pragma region String PrintF

_Success_(return > 0)
FORCEINLINE
ULONG
Str_VPrintfExW(
    _Out_writes_(BufferCount) _Always_(_Post_z_) PWSTR Buffer,
    _In_ ULONG BufferCount,
    _In_ _Printf_format_string_ PCWSTR Format,
    _In_opt_ va_list ArgList)
{
    return StrSafe_CchVPrintfW(Buffer, BufferCount, Format, ArgList);
}

_Success_(return > 0)
FORCEINLINE
ULONG
Str_VPrintfExA(
    _Out_writes_(BufferCount) _Always_(_Post_z_) PSTR Buffer,
    _In_ ULONG const BufferCount,
    _In_ _Printf_format_string_ PCSTR Format,
    _In_opt_ va_list ArgList)
{
    return StrSafe_CchVPrintfA(Buffer, BufferCount, Format, ArgList);
}

#define Str_PrintfExW StrSafe_CchPrintfW
#define Str_PrintfExA StrSafe_CchPrintfA

#define Str_PrintfW(Dest, Format, ...) Str_PrintfExW(Dest, ARRAYSIZE(Dest), Format, __VA_ARGS__)
#define Str_PrintfA(Dest, Format, ...) Str_PrintfExA(Dest, ARRAYSIZE(Dest), Format, __VA_ARGS__)
#define Str_VPrintfW(Dest, Format, ArgList) Str_VPrintfExW(Dest, ARRAYSIZE(Dest), Format, ArgList)
#define Str_VPrintfA(Dest, Format, ArgList) Str_VPrintfExA(Dest, ARRAYSIZE(Dest), Format, ArgList)

#pragma endregion Str_[V]Printf[Ex][A/W]

#pragma region String Copy

_Success_(return > 0)
FORCEINLINE
ULONG
Str_CopyExW(
    _Out_writes_(BufferCount) _Always_(_Post_z_) PWSTR Buffer,
    _In_range_(>, 0) ULONG BufferCount,
    _In_ PCWSTR Source)
{
    return StrSafe_CchCopyW(Buffer, BufferCount, Source);
}

_Success_(return > 0)
FORCEINLINE
ULONG
Str_CopyExA(
    _Out_writes_(BufferCount) _Always_(_Post_z_) PSTR Buffer,
    _In_range_(>, 0) ULONG BufferCount,
    _In_ PCSTR Source)
{
    return StrSafe_CchCopyA(Buffer, BufferCount, Source);
}

#define Str_CopyW(Dest, Source) Str_CopyExW(Dest, ARRAYSIZE(Dest), Source)
#define Str_CopyA(Dest, Source) Str_CopyExA(Dest, ARRAYSIZE(Dest), Source)

#pragma endregion Str_Copy[Ex][A/W]

#pragma region String Concatenation

_Success_(return > 0)
FORCEINLINE
ULONG
Str_CatExW(
    _Inout_updates_(BufferCount) PWSTR Buffer,
    _In_range_(>, 0) ULONG BufferCount,
    _In_ PCWSTR Source)
{
    ULONG i = (ULONG)Str_LenW(Buffer);
    return StrSafe_CchCopyW(Buffer + i, BufferCount - i, Source);
}

_Success_(return > 0)
FORCEINLINE
ULONG
Str_CatExA(
    _Inout_updates_(BufferCount) PSTR Buffer,
    _In_range_(>, 0) ULONG BufferCount,
    _In_ PCSTR Source)
{
    ULONG i = (ULONG)Str_LenA(Buffer);
    return StrSafe_CchCopyA(Buffer + i, BufferCount - i, Source);
}

#define Str_CatW(Buffer, Source) Str_CatExW(Buffer, ARRAYSIZE(Buffer), Source)
#define Str_CatA(Buffer, Source) Str_CatExA(Buffer, ARRAYSIZE(Buffer), Source)

#pragma endregion Str_Cat[Ex][A/W]
