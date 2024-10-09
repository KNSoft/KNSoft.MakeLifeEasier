#pragma once

#include <KNSoft/NDK/Package/StrSafe.h>

#include "../MakeLifeEasier.h"

EXTERN_C_START

#pragma region CRT Alias

#define Str_LenW wcslen
#define Str_LenA strlen
#ifdef UNICODE
#define Str_Len Str_LenW
#else
#define Str_Len Str_LenA
#endif

#define Str_StrW wcsstr
#define Str_StrA strstr
#ifdef UNICODE
#define Str_Str Str_StrW
#else
#define Str_Str Str_StrA
#endif

#pragma endregion

#pragma region String PrintF

#define Str_VPrintfExW StrSafe_CchVPrintfW
#define Str_VPrintfExA StrSafe_CchVPrintfA
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

#define Str_CopyExW StrSafe_CchCopyW
#define Str_CopyExA StrSafe_CchCopyA
#ifdef UNICODE
#define Str_CopyEx Str_CopyExW
#else
#define Str_CopyEx Str_CopyExA
#endif

#define Str_CopyW(Dest, Source) Str_CopyExW(Dest, ARRAYSIZE(Dest), Source)
#define Str_CopyA(Dest, Source) Str_CopyExA(Dest, ARRAYSIZE(Dest), Source)
#define Str_Copy(Dest, Source) Str_CopyEx(Dest, ARRAYSIZE(Dest), Source)

#pragma endregion Str_Copy[Ex][A/W]

/* Str_Size */
#define Str_SizeW(String) (Str_LenW(String) * sizeof(WCHAR))
#define Str_SizeA Str_LenA
#ifdef UNICODE
#define Str_Size Str_SizeW
#else
#define Str_Size Str_SizeA
#endif

EXTERN_C_END
