#include "../MakeLifeEasier.inl"

NTSTATUS
NTAPI
Sys_EnumPreferredLanguages(
    _In_ PSYS_ENUM_PREFERRED_LANGUAGES_FN Callback)
{
    NTSTATUS Status;
    ULONG Count, Length = 0, Flags;
    PZZWSTR Languages, Language;
    PCWSTR pszLangName;
    WCHAR wcName[LOCALE_NAME_MAX_LENGTH];
    UNICODE_STRING usLocaleName;

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
        usLocaleName.Length = 0;
        usLocaleName.MaximumLength = sizeof(wcName);
        usLocaleName.Buffer = wcName;
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
                Status = RtlGetParentLocaleName(pszLangName, &usLocaleName, 6, FALSE); // FIXME: Hard-coded flag 6
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

NTSTATUS
NTAPI
Sys_QueryInfo(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _Out_ PVOID* Info)
{
    PVOID Buffer;
    ULONG Length;
    NTSTATUS Status;

    Status = NtQuerySystemInformation(SystemInformationClass, NULL, 0, &Length);
    if (!NT_SUCCESS(Status) && Status != STATUS_INFO_LENGTH_MISMATCH)
    {
        return Status;
    }

_Try_Alloc:
    Buffer = RtlAllocateHeap(NtGetProcessHeap(), 0, Length);
    if (Buffer == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    Status = NtQuerySystemInformation(SystemInformationClass, Buffer, Length, &Length);
    if (NT_SUCCESS(Status))
    {
        *Info = Buffer;
        return STATUS_SUCCESS;
    }
    Mem_Free(Buffer);
    if (Status != STATUS_INFO_LENGTH_MISMATCH)
    {
        return Status;
    }
    goto _Try_Alloc;
}
