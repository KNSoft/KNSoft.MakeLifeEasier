#include "MakeLifeEasier.inl"

#pragma region I18N Support

static RTL_RUN_ONCE g_I18N_RunOnce = RTL_RUN_ONCE_INIT;

static
_Function_class_(RTL_RUN_ONCE_INIT_FN)
_IRQL_requires_same_
LOGICAL
NTAPI
Mlep_InitializeI18N(
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _Inout_opt_ PVOID Parameter,
    _Inout_opt_ PVOID *Context)
{
    NTSTATUS Status;
    ULONG Count, Length = 0;
    PZZWSTR Languages, Language;

    Status = RtlGetUserPreferredUILanguages(MUI_LANGUAGE_NAME, NULL, &Count, NULL, &Length);
    if (NT_SUCCESS(Status))
    {
        Status = Mem_Alloc(&Languages, Length * sizeof(WCHAR));
        if (NT_SUCCESS(Status))
        {
            Status = RtlGetUserPreferredUILanguages(MUI_LANGUAGE_NAME, NULL, &Count, Languages, &Length);
            if (NT_SUCCESS(Status))
            {
                Language = Languages;
                while (Language[0] != UNICODE_NULL)
                {
                    if (Precomp4C_I18N_SetCurrentLocale(&I18N_Table_All, Language) >= 0)
                    {
                        break;
                    }
                    while (*(Language++) != UNICODE_NULL);
                }
                if (Language[0] == UNICODE_NULL)
                {
                    Precomp4C_I18N_SetCurrentLocale(&I18N_Table_All, NULL);
                }
            }
        }
        Mem_Free(Languages);
    }

    return TRUE;
}

PCWSTR
Mlep_GetString(
    _In_ INT Index)
{
#pragma warning(disable: __WARNING_PROBE_NO_TRY)
    RtlRunOnceExecuteOnce(&g_I18N_RunOnce, Mlep_InitializeI18N, NULL, NULL);
#pragma warning(default: __WARNING_PROBE_NO_TRY)
    return Precomp4C_I18N_GetString(&I18N_Table_All, Index);
}

#pragma endregion

/* Test code */

#ifdef MSB_CONFIGURATIONTYPE_EXE

int
_cdecl
wmain(
    _In_ int argc,
    _In_reads_(argc) _Pre_z_ wchar_t** argv)
{
    return 0;
}

#endif
