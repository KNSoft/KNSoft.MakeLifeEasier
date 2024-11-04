#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

#define SYS_ENUM_PREFERRED_LOCALE_FALLBACK_PARENT  1
#define SYS_ENUM_PREFERRED_LOCALE_FALLBACK_LIST    2

// Return FALSE to stop enumeration
typedef
_Function_class_(SYS_ENUM_PREFERRED_LANGUAGES_FN)
LOGICAL
CALLBACK
SYS_ENUM_PREFERRED_LANGUAGES_FN(
    _In_ PCWSTR LanguageName,
    _In_ ULONG FallbackFlags);
typedef SYS_ENUM_PREFERRED_LANGUAGES_FN *PSYS_ENUM_PREFERRED_LANGUAGES_FN;

MLE_API
NTSTATUS
NTAPI
Sys_EnumPreferredLanguages(
    _In_ PSYS_ENUM_PREFERRED_LANGUAGES_FN Callback);

/* *Info should be freed by Sys_FreeInfo */
NTSTATUS
NTAPI
Sys_QueryInfo(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _Out_ PVOID* Info);

FORCEINLINE
LOGICAL
NTAPI
Sys_FreeInfo(
    _In_ PVOID Info)
{
    return RtlFreeHeap(NtGetProcessHeap(), 0, Info);
}

EXTERN_C_END
