#include "../MakeLifeEasier.inl"

NTSTATUS
NTAPI
Sys_EnumPreferredLanguages(
    _In_ PSYS_ENUM_PREFERRED_LANGUAGES_FN Callback)
{
    NTSTATUS Status;
    ULONG Count, Length = 0, Flags;
    PZZWSTR Languages, Language;
    WCHAR wcName[LOCALE_NAME_MAX_LENGTH];
    PCWSTR pszLangName;
    UNICODE_STRING usLocaleName = {
        .Length = 0,
        .MaximumLength = sizeof(wcName),
        .Buffer = wcName
    };

    Status = RtlGetThreadPreferredUILanguages(MUI_LANGUAGE_NAME, &Count, NULL, &Length);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Languages = Mem_Alloc(Length * sizeof(WCHAR));
    if (Languages == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    Status = RtlGetThreadPreferredUILanguages(MUI_LANGUAGE_NAME, &Count, Languages, &Length);
    if (NT_SUCCESS(Status))
    {
        Flags = 0;
        Language = Languages;
        while (Language[0] != UNICODE_NULL)
        {
            pszLangName = Language;
            do
            {
                if (!Callback(pszLangName, Flags))
                {
                    Status = STATUS_SUCCESS;
                    goto _Exit;
                }
                Status = RtlGetParentLocaleName(pszLangName, &usLocaleName, 6, FALSE); // FIXME: 6
                pszLangName = wcName;
                Flags &= SYS_ENUM_PREFERRED_LOCALE_FALLBACK_PARENT;
            } while (NT_SUCCESS(Status) && wcName[0] != UNICODE_NULL);
            while (*(Language++) != UNICODE_NULL);
            Flags = SYS_ENUM_PREFERRED_LOCALE_FALLBACK_LIST;
        }
    }
    Status = STATUS_NO_MORE_ENTRIES;

_Exit:
    Mem_Free(Languages);
    return Status;
}
