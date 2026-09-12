#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

MLE_API
W32ERROR
NTAPI
Sys_GetSessionToken(
    _Out_ PHANDLE TokenHandle,
    _In_ DWORD SessionId);

EXTERN_C_END
