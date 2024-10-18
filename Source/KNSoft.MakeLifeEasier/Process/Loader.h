#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

MLE_API
NTSTATUS
NTAPI
PS_GetProcAddress(
    _In_ PVOID DllBase,
    _In_ PCANSI_STRING Name,
    _Out_ PVOID* Address);

EXTERN_C_END
