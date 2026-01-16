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
Sys_GetLsaProcessId(
    _Out_ PULONG LsaProcessId);

FORCEINLINE
ULONG
Sys_GetCsrProcessId(VOID)
{
    return (ULONG)(ULONG_PTR)CsrGetProcessId();
}

_Success_(return != IMAGE_FILE_MACHINE_UNKNOWN)
FORCEINLINE
USHORT
Sys_ConvertArchToMachineType(
    _In_ USHORT ProcessorArch)
{
    if (ProcessorArch == PROCESSOR_ARCHITECTURE_INTEL)
    {
        return IMAGE_FILE_MACHINE_I386;
    } else if (ProcessorArch == PROCESSOR_ARCHITECTURE_AMD64)
    {
        return IMAGE_FILE_MACHINE_AMD64;
    } else if (ProcessorArch == PROCESSOR_ARCHITECTURE_ARM64)
    {
        return IMAGE_FILE_MACHINE_ARM64;
    }

    return IMAGE_FILE_MACHINE_UNKNOWN;
}

_Success_(return != IMAGE_FILE_MACHINE_UNKNOWN)
FORCEINLINE
USHORT
Sys_GetMachineType(VOID)
{
    if (IS_NT_VERSION_GE(NT_VERSION_WIN8))
    {
        return Sys_ConvertArchToMachineType(SharedUserData->NativeProcessorArchitecture);
    }

    /* Before Win10, only has x86 on x64 Wow */
    return PS_IsWow() ? IMAGE_FILE_MACHINE_I386 : IMAGE_FILE_MACHINE_AMD64;
}

EXTERN_C_END
