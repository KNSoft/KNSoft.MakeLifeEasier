#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

#pragma region Browser

typedef struct _NET_BROWSER_INFO
{
    PCWSTR Name;                    /* display name, e.g. L"Chrome" */
    PCWSTR Vendor;                  /* directory under LOCALAPPDATA */
    PCWSTR ExeName;                 /* chrome.exe / msedge.exe */
    WCHAR ExePath[MAX_PATH];        /* full path of the browser executable */
    WCHAR UserDataDir[MAX_PATH];    /* %LOCALAPPDATA%\<Vendor>\User Data */
} NET_BROWSER_INFO, *PNET_BROWSER_INFO;

typedef struct _NET_BROWSER_PROFILE
{
    WCHAR Directory[MAX_PATH];      /* L"Default", L"Profile 1", ... */
    WCHAR Name[MAX_PATH];           /* display name from Local State */
} NET_BROWSER_PROFILE, *PNET_BROWSER_PROFILE;

/* Enumerates installed Chromium-based browsers; caller frees *Browsers with Mem_Free */
MLE_API
NTSTATUS
NTAPI
Net_BrowserEnumerate(
    _Outptr_result_buffer_maybenull_(*Count) PNET_BROWSER_INFO* Browsers,
    _Out_ PULONG Count);

/* Enumerates the profiles of a browser User Data directory; display names come
   from the profile.info_cache node of Local State. Requires an initialized
   COM/WinRT apartment on the calling thread; caller frees *Profiles with Mem_Free */
MLE_API
NTSTATUS
NTAPI
Net_BrowserEnumerateProfiles(
    _In_ PCWSTR UserDataDir,
    _Outptr_result_buffer_maybenull_(*Count) PNET_BROWSER_PROFILE* Profiles,
    _Out_ PULONG Count);

#pragma endregion

EXTERN_C_END
