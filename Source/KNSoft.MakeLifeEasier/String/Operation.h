#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

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

EXTERN_C_END
