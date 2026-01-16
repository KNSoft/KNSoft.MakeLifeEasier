#pragma once

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
    _In_ PCWSTR Type,
    _In_ PCWSTR Name,
    _In_ WORD Language,
    _Out_ PVOID* Resource,
    _Out_opt_ PULONG Length)
{
    NTSTATUS Status;
    PIMAGE_RESOURCE_DATA_ENTRY DataEntry;
    ULONG_PTR Path[] = { (ULONG_PTR)Type, (ULONG_PTR)Name, (ULONG_PTR)Language };

    Status = LdrFindResource_U(Base, Path, ARRAYSIZE(Path), &DataEntry);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    return LdrAccessResource(Base, DataEntry, Resource, Length);
}

EXTERN_C_END
