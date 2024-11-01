#include "MakeLifeEasier.inl"

#pragma region I18N Support

static PS_RUNONCE g_I18N_Init = PS_RUNONCE_INIT;

static
_Function_class_(SYS_ENUM_PREFERRED_LANGUAGES_FN)
LOGICAL
CALLBACK
Mlep_EnumPreferredLang(
    _In_ PCWSTR LanguageName,
    _In_ ULONG FallbackFlags)
{
    return Precomp4C_I18N_SetCurrentLocale(&Precomp4C_I18N_Table_All, LanguageName) < 0;
}

static
FORCEINLINE
VOID
Mlep_InitializeI18N(VOID)
{
    if (PS_RunOnceBegin(&g_I18N_Init))
    {
        /* Choose system preferred language */
        if (Sys_EnumPreferredLanguages(Mlep_EnumPreferredLang) != STATUS_SUCCESS)
        {
            /* Fallback to the default */
            Precomp4C_I18N_SetCurrentLocale(&Precomp4C_I18N_Table_All, NULL);
        }
        PS_RunOnceEnd(&g_I18N_Init, TRUE);
    }
}

PCWSTR
Mlep_GetString(
    _In_ INT Index)
{
    Mlep_InitializeI18N();
    return Precomp4C_I18N_GetString(&Precomp4C_I18N_Table_All, Index);
}

#pragma endregion

#pragma region Dialog Box Support

W32ERROR
Mlep_DlgBox(
    _In_ PCWSTR DlgResName,
    _In_opt_ HWND Owner,
    _In_opt_ DLGPROC DlgProc,
    _In_opt_ LPARAM InitParam,
    _Out_opt_ PINT_PTR Result)
{
    W32ERROR Ret;
    PVOID Res;
    ULONG Len;
    INT_PTR DlgRet;
    DPI_AWARENESS_CONTEXT DPIContext;

    /* Access resource embedded by KNSoft.Precomp4C */
    if (!Precomp4C_Res2C_AccessResource(Precomp4C_Res2C_Resource_Embedded,
                                        ARRAYSIZE(Precomp4C_Res2C_Resource_Embedded),
                                        MAKEINTRESOURCEW(RT_DIALOG),
                                        DlgResName,
                                        LANG_USER_DEFAULT,
                                        &Res,
                                        &Len))
    {
        return ERROR_RESOURCE_DATA_NOT_FOUND;
    }

    /* All dialog boxes are DPI awared */
    DPIContext = UI_EnableDPIAwareContext();
    DlgRet = DialogBoxIndirectParamW((HINSTANCE)&__ImageBase, Res, Owner, DlgProc, InitParam);
    UI_RestoreDPIAwareContext(DPIContext);

    /* Handle return code */
    if (DlgRet == -1)
    {
        Ret = NtGetLastError();
    } else
    {
        if (Result != NULL)
        {
            *Result = DlgRet;
        }
        Ret = ERROR_SUCCESS;
    }
    return Ret;
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
