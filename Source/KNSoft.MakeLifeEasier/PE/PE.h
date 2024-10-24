﻿#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

FORCEINLINE
_Success_(return != NULL)
PCWSTR
PE_FindMessage(
    _In_ PVOID DllHandle,
    _In_ ULONG LanguageId,
    _In_ ULONG MessageId)
{
    PMESSAGE_RESOURCE_ENTRY lpstMRE;

    return NT_SUCCESS(
        RtlFindMessage(DllHandle,
                       (ULONG)(ULONG_PTR)RT_MESSAGETABLE,
                       LanguageId,
                       MessageId,
                       &lpstMRE)) && (lpstMRE->Flags & MESSAGE_RESOURCE_UNICODE) ? (PCWSTR)lpstMRE->Text : NULL;
}

FORCEINLINE
NTSTATUS
PE_AccessResource(
    _In_ PVOID Base,
    _In_ PLDR_RESOURCE_INFO Info,
    _Out_ PVOID* Resource,
    _Out_ PULONG Length)
{
    NTSTATUS Status;
    PIMAGE_RESOURCE_DATA_ENTRY DataEntry;

    Status = LdrFindResource_U(Base, Info, RESOURCE_DATA_LEVEL, &DataEntry);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    return LdrAccessResource(Base, DataEntry, Resource, Length);
}

EXTERN_C_END