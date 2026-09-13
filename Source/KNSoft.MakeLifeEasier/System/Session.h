#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

MLE_API
W32ERROR
NTAPI
Sys_GetSessionToken(
    _In_ DWORD SessionId,
    _Out_ PHANDLE TokenHandle);

EXTERN_C_END
