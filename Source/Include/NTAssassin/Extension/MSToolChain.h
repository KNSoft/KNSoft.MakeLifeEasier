#pragma once

#include "../NT/MinDef.h"

#pragma region MSVC

EXTERN_C_START

extern IMAGE_DOS_HEADER __ImageBase;

EXTERN_C_END

#if _WIN64
#define IS_WIN64 TRUE
#else
#define IS_WIN64 FALSE
#endif

/* Patch _STATIC_ASSERT to avoid confusion amount static_assert, _Static_assert and C_ASSERT */

#undef _STATIC_ASSERT
#define _STATIC_ASSERT(expr) static_assert((expr), #expr)

#define _A2U8(quote) __A2U8(quote)
#define __A2U8(quote) u8##quote

#define _A2W(quote) __A2W(quote)
#define __A2W(quote) L##quote

#pragma endregion

#pragma region MSBuild

#if defined(_M_IX86)
#define MSB_PLATFORMTARGET "x86"
#elif defined(_M_X64)
#define MSB_PLATFORMTARGET "x64"
#elif defined(_M_ARM64)
#define MSB_PLATFORMTARGET "ARM64"
#endif

#if defined(_DEBUG)
#define MSB_CONFIGURATION "Debug"
#define MSB_IS_DEBUG TRUE
#else
#define MSB_CONFIGURATION "Release"
#define MSB_IS_DEBUG FALSE
#endif

#define MSB_LIB_PATH(LibName) (MSB_PLATFORMTARGET "/" MSB_CONFIGURATION "/" ##LibName)

#pragma endregion

#pragma region WinSDK

#define DECLSPEC_EXPORT __declspec(dllexport)
typedef unsigned __int64 QWORD, near *PQWORD, far *LPQWORD;

// Makes a DWORD value by LOWORD and HIWORD
#define MAKEDWORD(l, h) ((DWORD)(((WORD)(((DWORD_PTR)(l)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(h)) & 0xffff))) << 16))
#define MAKEQWORD(l, h) ((QWORD)(((DWORD)(((DWORD_PTR)(l)) & 0xffffffff)) | ((QWORD)((DWORD)(((DWORD_PTR)(h)) & 0xffffffff))) << 32))

#pragma endregion
