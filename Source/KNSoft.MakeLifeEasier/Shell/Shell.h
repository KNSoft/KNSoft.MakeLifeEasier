﻿#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

MLE_API
W32ERROR
NTAPI
Shell_Exec(
    _In_z_ PCWSTR File,
    _In_opt_ PCWSTR Param,
    _In_opt_z_ PCWSTR Verb,
    _In_ INT ShowCmd,
    _Out_opt_ PHANDLE ProcessHandle);

MLE_API
HRESULT
NTAPI
Shell_LocateItem(
    _In_z_ PCWSTR Path);

MLE_API
HRESULT
NTAPI
Shell_GetLinkPath(
    _In_z_ PCWSTR LinkFile,
    _Out_writes_z_(PathCchSize) PWSTR Path,
    _In_ INT PathCchSize);

EXTERN_C_END
