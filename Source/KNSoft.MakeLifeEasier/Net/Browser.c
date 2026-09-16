#include "../MakeLifeEasier.inl"

#include <roapi.h>

#pragma comment(lib, "runtimeobject.lib")

/* known Chromium-based browsers */
static const struct
{
    PCWSTR Name;
    PCWSTR Vendor;
    PCWSTR ExeName;
}
Browser_Table[] =
{
    { L"Chrome", L"Google\\Chrome", L"chrome.exe" },
    { L"Edge",   L"Microsoft\\Edge", L"msedge.exe" },
};

/* static reference HSTRING, valid while Name lives */
FORCEINLINE
HRESULT
Browser_RefString(
    _In_ PCWSTR Name,
    _Out_ HSTRING_HEADER* Header,
    _Out_ HSTRING* String)
{
    return _Inline_WindowsCreateStringReference(Name, (ULONG)(Str_SizeW(Name) / sizeof(WCHAR)), Header, String);
}

static
HRESULT
Browser_GetNamedObject(
    _In_ IJsonObject* Object,
    _In_ PCWSTR Name,
    _Outptr_ IJsonObject** Value)
{
    HSTRING_HEADER Header;
    HSTRING String;
    HRESULT Hr;

    Hr = Browser_RefString(Name, &Header, &String);
    if (FAILED(Hr))
    {
        return Hr;
    }
    return Object->lpVtbl->GetNamedObject(Object, String, Value);
}

static
HRESULT
Browser_GetNamedString(
    _In_ IJsonObject* Object,
    _In_ PCWSTR Name,
    _Out_ HSTRING* Value)
{
    HSTRING_HEADER Header;
    HSTRING String;
    HRESULT Hr;

    Hr = Browser_RefString(Name, &Header, &String);
    if (FAILED(Hr))
    {
        return Hr;
    }
    return Object->lpVtbl->GetNamedString(Object, String, Value);
}

NTSTATUS
NTAPI
Net_BrowserEnumerate(
    _Outptr_result_buffer_maybenull_(*Count) PNET_BROWSER_INFO* Browsers,
    _Out_ PULONG Count)
{
    static UNICODE_STRING LocalAppData = RTL_CONSTANT_STRING(L"LOCALAPPDATA");
    static UNICODE_STRING Dirs[] =
    {
        RTL_CONSTANT_STRING(L"ProgramFiles"),
        RTL_CONSTANT_STRING(L"ProgramFiles(x86)"),
        RTL_CONSTANT_STRING(L"LOCALAPPDATA"),
    };
    NET_BROWSER_INFO Found[ARRAYSIZE(Browser_Table)];
    FILE_NETWORK_OPEN_INFORMATION Attributes;
    UNICODE_STRING Value;
    WCHAR Env[MAX_PATH];
    ULONG i, j, FoundCount = 0;

    *Browsers = NULL;
    *Count = 0;
    Value.Length = 0;
    Value.MaximumLength = sizeof(Env);
    Value.Buffer = Env;
    if (!NT_SUCCESS(RtlQueryEnvironmentVariable_U(NULL, &LocalAppData, &Value)))
    {
        return STATUS_UNSUCCESSFUL;
    }
    Env[Value.Length / sizeof(WCHAR)] = UNICODE_NULL;

    for (i = 0; i < ARRAYSIZE(Browser_Table); i++)
    {
        PNET_BROWSER_INFO Info = &Found[FoundCount];
        BOOL Installed = FALSE;

        Str_PrintfExW(Info->UserDataDir,
                      MAX_PATH,
                      L"%ls\\%ls\\User Data",
                      Env,
                      Browser_Table[i].Vendor);
        if (!NT_SUCCESS(IO_GetWin32FileAttributes(Info->UserDataDir, NULL, &Attributes)) ||
            !BooleanFlagOn(Attributes.FileAttributes, FILE_ATTRIBUTE_DIRECTORY))
        {
            continue;
        }
        for (j = 0; j < ARRAYSIZE(Dirs) && !Installed; j++)
        {
            WCHAR Path[MAX_PATH];

            Value.Length = 0;
            Value.MaximumLength = sizeof(Path);
            Value.Buffer = Path;
            if (!NT_SUCCESS(RtlQueryEnvironmentVariable_U(NULL, &Dirs[j], &Value)))
            {
                continue;
            }
            Path[Value.Length / sizeof(WCHAR)] = UNICODE_NULL;
            Str_PrintfExW(Info->ExePath,
                          MAX_PATH,
                          L"%ls\\%ls\\Application\\%ls",
                          Path,
                          Browser_Table[i].Vendor,
                          Browser_Table[i].ExeName);
            if (NT_SUCCESS(IO_GetWin32FileAttributes(Info->ExePath, NULL, &Attributes)) &&
                !BooleanFlagOn(Attributes.FileAttributes, FILE_ATTRIBUTE_DIRECTORY))
            {
                Installed = TRUE;
            }
        }
        if (!Installed)
        {
            continue;
        }

        Info->Name = Browser_Table[i].Name;
        Info->Vendor = Browser_Table[i].Vendor;
        Info->ExeName = Browser_Table[i].ExeName;
        FoundCount++;
    }

    if (FoundCount != 0)
    {
        *Browsers = Mem_Alloc(FoundCount * sizeof(**Browsers));
        if (*Browsers == NULL)
        {
            return STATUS_NO_MEMORY;
        }
        RtlCopyMemory(*Browsers, Found, FoundCount * sizeof(**Browsers));
        *Count = FoundCount;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
Net_BrowserEnumerateProfiles(
    _In_ PCWSTR UserDataDir,
    _Outptr_result_buffer_maybenull_(*Count) PNET_BROWSER_PROFILE* Profiles,
    _Out_ PULONG Count)
{
    PNET_BROWSER_PROFILE List = NULL;
    IJsonValue* Root = NULL;
    IJsonObject* RootObject = NULL, * InfoCache = NULL;
    PFILE_ID_EXTD_DIR_INFORMATION Entry;
    FILE_FIND Find;
    OBJECT_ATTRIBUTES Object;
    UNICODE_STRING NtPath;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_NETWORK_OPEN_INFORMATION Attributes;
    WCHAR LocalState[MAX_PATH], Candidate[MAX_PATH];
    ULONG Count_ = 0, Capacity = 0, NameLength;
    NTSTATUS Status;

    *Profiles = NULL;
    *Count = 0;

    /* optional display-name map: profile.info_cache.<dir>.name */
    Str_PrintfExW(LocalState,
                  MAX_PATH,
                  L"%ls\\Local State",
                  UserDataDir);
    if (SUCCEEDED(Data_JsonParseUtf8File(LocalState, 1 << 20, &Root)))
    {
        IJsonObject* ProfileObject;

        if (SUCCEEDED(Root->lpVtbl->GetObject(Root, &RootObject)) &&
            SUCCEEDED(Browser_GetNamedObject(RootObject, L"profile", &ProfileObject)))
        {
            Browser_GetNamedObject(ProfileObject, L"info_cache", &InfoCache);
            ProfileObject->lpVtbl->Release(ProfileObject);
        }
    }

    Status = NT_Win32PathToNtPath(UserDataDir, NULL, &NtPath);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    NT_InitObject(&Object, &NtPath, OBJ_CASE_INSENSITIVE, NULL);
    Status = NtOpenFile(&Find.DirectoryHandle,
                        FILE_LIST_DIRECTORY | SYNCHRONIZE,
                        &Object,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    NT_FreeNtPath(&NtPath);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = IO_BeginFindFile(&Find, Find.DirectoryHandle, NULL, FileIdExtdDirectoryInformation);
    if (!NT_SUCCESS(Status))
    {
        NtClose(Find.DirectoryHandle);
        goto Cleanup;
    }

    Status = STATUS_SUCCESS;
    while (Find.HasData && NT_SUCCESS(Status))
    {
        Entry = Find.Buffer;
        for (;;)
        {
            NameLength = Entry->FileNameLength / sizeof(WCHAR);
            if (BooleanFlagOn(Entry->FileAttributes, FILE_ATTRIBUTE_DIRECTORY) &&
                NameLength != 0 && NameLength < MAX_PATH &&
                !(NameLength == 1 && Entry->FileName[0] == L'.') &&
                !(NameLength == 2 && Entry->FileName[0] == L'.' && Entry->FileName[1] == L'.'))
            {
                /* a profile directory contains a "Preferences" file */
                RtlCopyMemory(Candidate, Entry->FileName, NameLength * sizeof(WCHAR));
                Candidate[NameLength] = UNICODE_NULL;
                Str_PrintfExW(LocalState,
                              MAX_PATH,
                              L"%ls\\%ls\\Preferences",
                              UserDataDir,
                              Candidate);
                if (NT_SUCCESS(IO_GetWin32FileAttributes(LocalState, NULL, &Attributes)))
                {
                    if (Count_ == Capacity)
                    {
                        PNET_BROWSER_PROFILE NewList;

                        Capacity = Capacity != 0 ? Capacity * 2 : 8;
                        NewList = Mem_ReAlloc(List, Capacity * sizeof(*List));
                        if (NewList == NULL)
                        {
                            Status = STATUS_NO_MEMORY;
                            break;
                        }
                        List = NewList;
                    }
                    Str_CopyExW(List[Count_].Directory, MAX_PATH, Candidate);
                    Str_CopyExW(List[Count_].Name, MAX_PATH, Candidate);
                    if (InfoCache != NULL)
                    {
                        IJsonObject* ProfileEntry;

                        /* info_cache.<dir> is an object; the display name is its "name" member */
                        if (SUCCEEDED(Browser_GetNamedObject(InfoCache, Candidate, &ProfileEntry)))
                        {
                            HSTRING Name = NULL;

                            if (SUCCEEDED(Browser_GetNamedString(ProfileEntry, L"name", &Name)))
                            {
                                PCWSTR Text = _Inline_WindowsGetStringRawBuffer(Name, NULL);

                                if (Text[0] != UNICODE_NULL)
                                {
                                    Str_PrintfExW(List[Count_].Name,
                                                  MAX_PATH,
                                                  L"%ls (%ls)",
                                                  List[Count_].Directory,
                                                  Text);
                                }
                                _Inline_WindowsDeleteString(Name);
                            }
                            ProfileEntry->lpVtbl->Release(ProfileEntry);
                        }
                    }
                    Count_++;
                }
            }
            if (Entry->NextEntryOffset == 0)
            {
                break;
            }
            Entry = (PFILE_ID_EXTD_DIR_INFORMATION)((PBYTE)Entry + Entry->NextEntryOffset);
        }
        if (!NT_SUCCESS(Status))
        {
            break;
        }
        Status = IO_ContinueFindFileFind(&Find);
    }
    IO_EndFindFile(&Find);
    NtClose(Find.DirectoryHandle);

    if (NT_SUCCESS(Status) && Count_ != 0)
    {
        *Profiles = Mem_Alloc(Count_ * sizeof(**Profiles));
        if (*Profiles == NULL)
        {
            Status = STATUS_NO_MEMORY;
        } else
        {
            RtlCopyMemory(*Profiles, List, Count_ * sizeof(**Profiles));
            *Count = Count_;
        }
    }

Cleanup:
    Mem_Free(List);
    if (InfoCache != NULL)
    {
        InfoCache->lpVtbl->Release(InfoCache);
    }
    if (RootObject != NULL)
    {
        RootObject->lpVtbl->Release(RootObject);
    }
    if (Root != NULL)
    {
        Root->lpVtbl->Release(Root);
    }
    return Status;
}
