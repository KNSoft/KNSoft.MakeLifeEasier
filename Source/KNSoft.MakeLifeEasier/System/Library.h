#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

typedef enum _SYS_LIB_INDEX
{
    SysLibNtDll = 0,
    SysLibKernel32,
    SysLibUser32,
    SysLibComDlg32,
    SysLibOle32,
    SysLibUxTheme,
    SysLibDwmapi,
    SysLibShcore,
    SysLibWs2_32,
    SysLibWinmm,
    SysLibMax
} SYS_LIB_INDEX, *PSYS_LIB_INDEX;

MLE_API
NTSTATUS
NTAPI
Sys_LoadDll(
    _In_ SYS_LIB_INDEX SysLib,
    _Out_ PVOID* DllBase);

EXTERN_C_END
