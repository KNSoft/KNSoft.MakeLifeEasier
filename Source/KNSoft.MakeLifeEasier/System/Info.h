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
    _In_ ULONG FallbackFlags,
    _In_opt_ PVOID Context);
typedef SYS_ENUM_PREFERRED_LANGUAGES_FN *PSYS_ENUM_PREFERRED_LANGUAGES_FN;

MLE_API
NTSTATUS
NTAPI
Sys_EnumPreferredLanguages(
    _In_ PSYS_ENUM_PREFERRED_LANGUAGES_FN Callback,
    _In_opt_ PVOID Context);

/* *Info should be freed by Sys_FreeInfo */
MLE_API
NTSTATUS
NTAPI
Sys_QueryDynamicInfo(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _Out_ PVOID* Info);

FORCEINLINE
LOGICAL
NTAPI
Sys_FreeInfo(
    __drv_freesMem(Mem) _Frees_ptr_ _Post_invalid_ PVOID Info)
{
    return RtlFreeHeap(RtlProcessHeap(), 0, Info);
}

MLE_API
NTSTATUS
NTAPI
Sys_LsaGetProcessId(
    _Out_ PULONG LsaProcessId);

EXTERN_C_END
