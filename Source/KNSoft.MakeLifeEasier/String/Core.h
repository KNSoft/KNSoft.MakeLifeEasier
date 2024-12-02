#pragma once

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

#ifdef UNICODE
#define Str_Len Str_LenW
#else
#define Str_Len Str_LenA
#endif

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

#ifdef UNICODE
#define Str_Cmp Str_CmpW
#else
#define Str_Cmp Str_CmpA
#endif

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

#ifdef UNICODE
#define Str_Str Str_StrW
#else
#define Str_Str Str_StrA
#endif

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

#ifdef UNICODE
#define Str_Size Str_SizeW
#else
#define Str_Size Str_SizeA
#endif

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

#ifdef UNICODE
#define Str_Equal Str_EqualW
#else
#define Str_Equal Str_EqualA
#endif

#pragma endregion

FORCEINLINE
LOGICAL
Str_TestCchRet(
    _In_ unsigned long CchRet,
    _In_ size_t const BufferCount)
{
    return CchRet > 0 && CchRet < BufferCount;
}

#pragma region String PrintF

_Success_(
    return > 0 && return < BufferCount
)
FORCEINLINE
ULONG
Str_VPrintfExW(
    _Out_writes_opt_(BufferCount) _Always_(_Post_z_) wchar_t* const Buffer,
    _In_ size_t const BufferCount,
    _In_z_ _Printf_format_string_ const wchar_t* Format,
    va_list ArgList)
{
    return StrSafe_CchVPrintfW(Buffer, BufferCount, Format, ArgList);
}

_Success_(
    return > 0 && return < BufferCount
)
FORCEINLINE
ULONG
Str_VPrintfExA(
    _Out_writes_opt_(BufferCount) _Always_(_Post_z_) char* const Buffer,
    _In_ size_t const BufferCount,
    _In_z_ _Printf_format_string_ const char* Format,
    va_list ArgList)
{
    return StrSafe_CchVPrintfA(Buffer, BufferCount, Format, ArgList);
}

#define Str_PrintfExW StrSafe_CchPrintfW
#define Str_PrintfExA StrSafe_CchPrintfA
#ifdef UNICODE
#define Str_VPrintfEx Str_VPrintfExW
#define Str_PrintfEx Str_PrintfExW
#else
#define Str_VPrintfEx Str_VPrintfExA
#define Str_PrintfEx Str_PrintfExA
#endif

#define Str_PrintfW(Dest, Format, ...) Str_PrintfExW(Dest, ARRAYSIZE(Dest), Format, __VA_ARGS__)
#define Str_PrintfA(Dest, Format, ...) Str_PrintfExA(Dest, ARRAYSIZE(Dest), Format, __VA_ARGS__)
#define Str_Printf(Dest, Format, ...) Str_PrintfEx(Dest, ARRAYSIZE(Dest), Format, __VA_ARGS__)
#define Str_VPrintfW(Dest, Format, ArgList) Str_VPrintfExW(Dest, ARRAYSIZE(Dest), Format, ArgList)
#define Str_VPrintfA(Dest, Format, ArgList) Str_VPrintfExA(Dest, ARRAYSIZE(Dest), Format, ArgList)
#define Str_VPrintf(Dest, Format, ArgList) Str_VPrintfEx(Dest, ARRAYSIZE(Dest), Format, ArgList)

#pragma endregion Str_[V]Printf[Ex][A/W]

#pragma region String Copy

_Success_(
    return > 0 && return < BufferCount
)
FORCEINLINE
SIZE_T
Str_CopyExW(
    _Out_writes_opt_(BufferCount) _When_(BufferCount > 0, _Notnull_) _Always_(_Post_z_) wchar_t* const Buffer,
    _In_ size_t const BufferCount,
    _In_z_ const wchar_t* Source)
{
    return StrSafe_CchCopyW(Buffer, BufferCount, Source);
}

_Success_(
    return > 0 && return < BufferCount
)
FORCEINLINE
SIZE_T
Str_CopyExA(
    _Out_writes_opt_(BufferCount) _When_(BufferCount > 0, _Notnull_) _Always_(_Post_z_) char* const Buffer,
    _In_ size_t const BufferCount,
    _In_z_ const char* Source)
{
    return StrSafe_CchCopyA(Buffer, BufferCount, Source);
}

#ifdef UNICODE
#define Str_CopyEx Str_CopyExW
#else
#define Str_CopyEx Str_CopyExA
#endif

#define Str_CopyW(Dest, Source) Str_CopyExW(Dest, ARRAYSIZE(Dest), Source)
#define Str_CopyA(Dest, Source) Str_CopyExA(Dest, ARRAYSIZE(Dest), Source)
#define Str_Copy(Dest, Source) Str_CopyEx(Dest, ARRAYSIZE(Dest), Source)

#pragma endregion Str_Copy[Ex][A/W]
