#include "../MakeLifeEasier.inl"

NTSTATUS
NTAPI
Sys_EnumPreferredLanguages(
    _In_ PSYS_ENUM_PREFERRED_LANGUAGES_FN Callback,
    _In_opt_ PVOID Context)
{
    NTSTATUS Status;
    ULONG Count, Length = 0, Flags;
    PZZWSTR Languages, Language;
    PCWSTR pszLangName;
    WCHAR wcLangName[LOCALE_NAME_MAX_LENGTH];
    UNICODE_STRING usLangName;

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
        usLangName.Length = 0;
        usLangName.MaximumLength = sizeof(wcLangName);
        usLangName.Buffer = wcLangName;
        Flags = 0;
        Language = Languages;
        while (Language[0] != UNICODE_NULL)
        {
            pszLangName = Language;
            do
            {
                if (!Callback(pszLangName, Flags, Context))
                {
                    Status = STATUS_SUCCESS;
                    goto _Exit;
                }
                Status = RtlGetParentLocaleName(pszLangName, &usLangName, 6, FALSE); // FIXME: Hard-coded flag 6
                pszLangName = wcLangName;
                Flags &= SYS_ENUM_PREFERRED_LOCALE_FALLBACK_PARENT;
            } while (NT_SUCCESS(Status) && wcLangName[0] != UNICODE_NULL);
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
Sys_QueryDynamicInfo(
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
    Buffer = RtlAllocateHeap(RtlProcessHeap(), 0, Length);
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

static UNICODE_STRING g_LsaKeyPath = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\Lsa");
static UNICODE_STRING g_LsaPidKeyName = RTL_CONSTANT_STRING(L"LsaPid");

NTSTATUS
NTAPI
Sys_GetLsaProcessId(
    _Out_ PULONG LsaProcessId)
{
    NTSTATUS Status;
    DWORD Pid;
    HANDLE KeyHandle;
    OBJECT_ATTRIBUTES ObjectAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(&g_LsaKeyPath, 0);

    Status = NtOpenKey(&KeyHandle, KEY_QUERY_VALUE, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = Sys_RegQueryDword(KeyHandle, &g_LsaPidKeyName, &Pid);
    NtClose(KeyHandle);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    *LsaProcessId = Pid;
    return STATUS_SUCCESS;
}
