#pragma once

#if defined(_KNSOFT_PRECOMP4C_I18N_)

#include "../MakeLifeEasier.h"

#include "../Process/Sync.h"
#include "../System/Info.h"

EXTERN_C_START

typedef struct _KNS_I18N_TABLE
{
    PS_RUNONCE InitOnce;
    PRECOMP4C_I18N_TABLE* Table;
} KNS_I18N_TABLE, *PKNS_I18N_TABLE;

static
FORCEINLINE
_Function_class_(SYS_ENUM_PREFERRED_LANGUAGES_FN)
LOGICAL
CALLBACK
KNS_I18NEnumPreferredLang(
    _In_ PCWSTR LanguageName,
    _In_ ULONG FallbackFlags,
    _In_opt_ PVOID Context)
{
    _Analysis_assume_(Context != NULL);
    return Precomp4C_I18N_SetCurrentLocale(((PKNS_I18N_TABLE)Context)->Table, LanguageName) < 0;
}

FORCEINLINE
VOID
KNS_I18NInitTable(
    _In_ PKNS_I18N_TABLE I18NTable)
{
    if (PS_RunOnceBegin(&I18NTable->InitOnce))
    {
        /* Choose system preferred language */
        if (Sys_EnumPreferredLanguages(KNS_I18NEnumPreferredLang, I18NTable) != STATUS_SUCCESS)
        {
            /* Fallback to the default */
            Precomp4C_I18N_SetCurrentLocale(I18NTable->Table, NULL);
        }
        PS_RunOnceEnd(&I18NTable->InitOnce, TRUE);
    }
}

FORCEINLINE
PCWSTR
KNS_I18NGetString(
    _In_ PKNS_I18N_TABLE I18NTable,
    _In_ INT Index)
{
    KNS_I18NInitTable(I18NTable);
    return Precomp4C_I18N_GetString(I18NTable->Table, Index);
}

FORCEINLINE
VOID
KNS_I18NInitArray(
    _In_ PKNS_I18N_TABLE I18NTable,
    _In_ PVOID Array,
    _In_ ULONG Size,
    _In_ ULONG Count,
    _In_ ULONG FieldOffset)
{
    PBYTE pStruct = (PBYTE)Array;
    PLONG_PTR pIndex;
    ULONG i;

    for (i = 0; i < Count; i++)
    {
        pIndex = (PLONG_PTR)Add2Ptr(pStruct, FieldOffset);
        *pIndex = (LONG_PTR)KNS_I18NGetString(I18NTable, (INT)*pIndex);
        pStruct = (PBYTE)Add2Ptr(pStruct, Size);
    }
}

EXTERN_C_END

#endif /* defined(_KNSOFT_PRECOMP4C_I18N_) */
