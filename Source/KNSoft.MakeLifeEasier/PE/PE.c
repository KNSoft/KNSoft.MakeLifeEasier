#include "../MakeLifeEasier.inl"

_Success_(return != NULL)
PCWSTR
NTAPI
PE_GetMessage(
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
