#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

MLE_API
_Success_(return != NULL)
PCWSTR
NTAPI
PE_GetMessage(
    _In_ PVOID DllHandle,
    _In_ ULONG LanguageId,
    _In_ ULONG MessageId);

EXTERN_C_END
