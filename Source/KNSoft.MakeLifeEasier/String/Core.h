#pragma once

#include <KNSoft/NDK/Package/StrSafe.inl>
#include <KNSoft/NDK/NDK.h>
#include <KNSoft/NDK/NT/Nls.inl>

#pragma region Case Conversion

/* ASCII only */

FORCEINLINE
CHAR
Str_UpperCharA(
    _In_ CHAR Char)
{
    return (CHAR)_Inline_NLS_UPCASE(NULL, (UCHAR)Char);
}

FORCEINLINE
CHAR
Str_LowerCharA(
    _In_ CHAR Char)
{
    return (CHAR)_Inline_NLS_DOWNCASE(NULL, (UCHAR)Char);
}

/* Non-ASCII characters are folded by RtlUpcase/DowncaseUnicodeChar */

FORCEINLINE
WCHAR
Str_UpperCharW(
    _In_ WCHAR Char)
{
    return Char < 0x80 ? (WCHAR)Str_UpperCharA((CHAR)Char) : RtlUpcaseUnicodeChar(Char);
}

FORCEINLINE
WCHAR
Str_LowerCharW(
    _In_ WCHAR Char)
{
    return Char < 0x80 ? (WCHAR)Str_LowerCharA((CHAR)Char) : RtlDowncaseUnicodeChar(Char);
}

/* In-place string conversion */

FORCEINLINE
VOID
Str_UpperA(
    _Inout_z_ PSTR String)
{
    while (*String != ANSI_NULL)
    {
        *String = Str_UpperCharA(*String);
        String++;
    }
}

FORCEINLINE
VOID
Str_LowerA(
    _Inout_z_ PSTR String)
{
    while (*String != ANSI_NULL)
    {
        *String = Str_LowerCharA(*String);
        String++;
    }
}

FORCEINLINE
VOID
Str_UpperW(
    _Inout_z_ PWSTR String)
{
    while (*String != UNICODE_NULL)
    {
        *String = Str_UpperCharW(*String);
        String++;
    }
}

FORCEINLINE
VOID
Str_LowerW(
    _Inout_z_ PWSTR String)
{
    while (*String != UNICODE_NULL)
    {
        *String = Str_LowerCharW(*String);
        String++;
    }
}

#pragma endregion

#pragma region Basic Operations

/* Str_Size */

FORCEINLINE
SIZE_T
Str_SizeW(
    _In_ PCWSTR String)
{
    return wcslen(String) * sizeof(WCHAR);
}

FORCEINLINE
SIZE_T
Str_SizeA(
    _In_ PCSTR String)
{
    return strlen(String) * sizeof(CHAR);
}

/* Str_Equal */

FORCEINLINE
LOGICAL
Str_EqualA(
    _In_ PCSTR String1,
    _In_ PCSTR String2)
{
    return strcmp(String1, String2) == 0;
}

FORCEINLINE
LOGICAL
Str_EqualW(
    _In_ PCWSTR String1,
    _In_ PCWSTR String2)
{
    return wcscmp(String1, String2) == 0;
}

/* Str_EqualI (case-insensitive) */

FORCEINLINE
LOGICAL
Str_EqualIA(
    _In_z_ PCSTR String1,
    _In_z_ PCSTR String2)
{
    while (*String1 != ANSI_NULL && Str_UpperCharA(*String1) == Str_UpperCharA(*String2))
    {
        String1++;
        String2++;
    }
    return Str_UpperCharA(*String1) == Str_UpperCharA(*String2);
}

FORCEINLINE
LOGICAL
Str_EqualIW(
    _In_z_ PCWSTR String1,
    _In_z_ PCWSTR String2)
{
    while (*String1 != UNICODE_NULL && Str_UpperCharW(*String1) == Str_UpperCharW(*String2))
    {
        String1++;
        String2++;
    }
    return Str_UpperCharW(*String1) == Str_UpperCharW(*String2);
}

/* Str_StrI: case-insensitive substring search, returns NULL if not found */

_Success_(return != NULL)
FORCEINLINE
PSTR
Str_StrIA(
    _In_z_ PCSTR String,
    _In_z_ PCSTR SubString)
{
    SIZE_T i, j;

    if (*SubString == ANSI_NULL)
    {
        return (PSTR)String;
    }

    for (i = 0; String[i] != ANSI_NULL; i++)
    {
        for (j = 0; ; j++)
        {
            if (SubString[j] == ANSI_NULL) return (PSTR)(String + i);
            if (Str_UpperCharA(String[i + j]) != Str_UpperCharA(SubString[j])) break;
        }
    }
    return NULL;
}

_Success_(return != NULL)
FORCEINLINE
PWSTR
Str_StrIW(
    _In_z_ PCWSTR String,
    _In_z_ PCWSTR SubString)
{
    SIZE_T i, j;

    if (*SubString == UNICODE_NULL)
    {
        return (PWSTR)String;
    }

    for (i = 0; String[i] != UNICODE_NULL; i++)
    {
        for (j = 0; ; j++)
        {
            if (SubString[j] == UNICODE_NULL) return (PWSTR)(String + i);
            if (Str_UpperCharW(String[i + j]) != Str_UpperCharW(SubString[j])) break;
        }
    }
    return NULL;
}

/* Str_[Index/StartsWith] */

FORCEINLINE
SIZE_T
Str_IndexW(
    _In_ PCWSTR String,
    _In_ PCWSTR SubString)
{
    PCWSTR pSubStr = wcsstr(String, SubString);
    return pSubStr != NULL ? pSubStr - String : -1;
}

FORCEINLINE
SIZE_T
Str_IndexA(
    _In_ PCSTR String,
    _In_ PCSTR SubString)
{
    PCSTR pSubStr = strstr(String, SubString);
    return pSubStr != NULL ? pSubStr - String : -1;
}

FORCEINLINE
LOGICAL
Str_StartsWithW(
    _In_ PCWSTR String,
    _In_ PCWSTR SubString)
{
    return Str_IndexW(String, SubString) == 0;
}

FORCEINLINE
LOGICAL
Str_StartsWithA(
    _In_ PCSTR String,
    _In_ PCSTR SubString)
{
    return Str_IndexA(String, SubString) == 0;
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
    ULONG i = (ULONG)wcslen(Buffer);
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
    ULONG i = (ULONG)strlen(Buffer);
    return StrSafe_CchCopyA(Buffer + i, BufferCount - i, Source);
}

#define Str_CatW(Buffer, Source) Str_CatExW(Buffer, ARRAYSIZE(Buffer), Source)
#define Str_CatA(Buffer, Source) Str_CatExA(Buffer, ARRAYSIZE(Buffer), Source)

#pragma endregion Str_Cat[Ex][A/W]
