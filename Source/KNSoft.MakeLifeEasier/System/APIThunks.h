#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

/*
 * Expand the DLL list and API list on demand
 */

/* The case follow the real file name */
#define _SYS_APITHUNKS_APPLY_DLLS \
    _APPLY(ntdll) \
    _APPLY(kernel32) \
    _APPLY(user32) \
    _APPLY(Dwmapi)

#define _SYS_APITHUNKS_APPLY_FUNCTIONS \
    _APPLY(ntdll, NtWow64GetNativeSystemInformation, 0) \
    _APPLY(ntdll, NtWow64ReadVirtualMemory64, 0) \
    _APPLY(user32, AdjustWindowRectExForDpi, NT_VERSION_WIN10_1607) \
    _APPLY(user32, AreDpiAwarenessContextsEqual, NT_VERSION_WIN10_1607) \
    _APPLY(user32, GetThreadDpiAwarenessContext, NT_VERSION_WIN10_1607) \
    _APPLY(user32, GetWindowDpiAwarenessContext, NT_VERSION_WIN10_1607) \
    _APPLY(user32, SetThreadDpiAwarenessContext, NT_VERSION_WIN10_1607) \
    _APPLY(user32, GetProcessDpiAwareness, NT_VERSION_WINBLUE)

#define _SYS_APITHUNKS_PREFIX_DLL(DLLName) SysDll_##DLLName

typedef enum _SYS_DLL_INDEX
{
#define _APPLY(DLLName) _SYS_APITHUNKS_PREFIX_DLL(DLLName),
    _SYS_APITHUNKS_APPLY_DLLS
#undef _APPLY
    SysDll_Max
} SYS_DLL_INDEX;
_STATIC_ASSERT(SysDll_ntdll == 0); // We used a quick way to obtain it

MLE_API EXTERN_C CONST UNICODE_STRING Sys_DLLNames[SysDll_Max];

MLE_API
_Success_(return != NULL)
PVOID
NTAPI
Sys_LoadDll(
    _In_ SYS_DLL_INDEX SysDll);

#define _SYS_APITHUNKS_PREFIX_FUNC(FuncName) SysFunc_##FuncName

typedef enum _SYS_FUNCTION_INDEX
{
#define _APPLY(DLLName, FuncName, MinNtVer) _SYS_APITHUNKS_PREFIX_FUNC(FuncName),
    _SYS_APITHUNKS_APPLY_FUNCTIONS
#undef _APPLY
    SysFunc_Max
} SYS_FUNCTION_INDEX;
_STATIC_ASSERT(SysFunc_Max <= MAXUCHAR); // We used a byte array to map it

MLE_API
_Success_(return != NULL)
PVOID
NTAPI
Sys_LoadFunction(
    _In_ SYS_FUNCTION_INDEX SysFunction);

#define SYS_DECL_LOAD_API(APIName) typeof(&APIName) pfn##APIName
#define SYS_GET_LOAD_API(APIName) ((typeof(&APIName))Sys_LoadFunction(_SYS_APITHUNKS_PREFIX_FUNC(APIName)))
#define SYS_LOAD_API(APIName) SYS_DECL_LOAD_API(APIName) = SYS_GET_LOAD_API(APIName)

EXTERN_C_END
