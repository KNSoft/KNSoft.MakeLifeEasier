#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

MLE_API
_Success_(return != NULL)
PCWSTR
NTAPI
Err_GetWin32ErrorInfo(
    _In_ ULONG Win32Error);

MLE_API
_Success_(return != NULL)
PCWSTR
NTAPI
Err_GetNtStatusInfo(
    _In_ NTSTATUS Status);

MLE_API
_Success_(return != NULL)
PCWSTR
NTAPI
Err_GetHResultInfo(
    _In_ HRESULT HResult);

EXTERN_C_END
