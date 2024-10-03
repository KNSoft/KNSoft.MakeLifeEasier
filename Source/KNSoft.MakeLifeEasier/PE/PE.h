#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

MLE_API
_Success_(return != NULL)
PCWSTR
NTAPI
PE_FindMessage(
    _In_ PVOID DllHandle,
    _In_ ULONG LanguageId,
    _In_ ULONG MessageId);

MLE_API
NTSTATUS
NTAPI
PE_AccessResource(
    _In_ PVOID Base,
    _In_ PLDR_RESOURCE_INFO Info,
    _Out_ PVOID* Resource,
    _Out_ PULONG Length);

EXTERN_C_END
