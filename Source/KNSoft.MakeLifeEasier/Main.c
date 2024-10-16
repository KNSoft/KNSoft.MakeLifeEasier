#include "MakeLifeEasier.inl"

#pragma region I18N Support

static _Interlocked_operand_ LONG volatile g_I18N_Init = FALSE;

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
VOID
Mlep_InitializeI18N(VOID)
{
    /* Initialize once */
    if (_InterlockedCompareExchange(&g_I18N_Init, TRUE, FALSE) != FALSE)
    {
        return;
    }

    /* Select preferred language */
    if (Sys_EnumPreferredLanguages(Mlep_EnumPreferredLang) != STATUS_SUCCESS)
    {
        Precomp4C_I18N_SetCurrentLocale(&Precomp4C_I18N_Table_All, NULL);
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
    DlgRet = DialogBoxIndirectParamW((HINSTANCE)&__ImageBase, Res, Owner, DlgProc, InitParam);
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

static UI_VALUEEDITOR_CONSTANT g_Consts[] = {
    { WS_BORDER, L"WS_BORDER", L"Window has border" },
    { WS_ACTIVECAPTION, L"WS_ACTIVECAPTION", L"Active caption" },
    { WS_DLGFRAME, L"WS_DLGFRAME", L"Window has frame" },
    { WS_CHILD, L"WS_CHILD", L"Window is a child" },
    { WS_DISABLED, L"WS_DISABLED", L"Window is disabled" },
    { WS_MINIMIZEBOX, L"WS_MINIMIZEBOX", L"Window has _" },
};

int
_cdecl
wmain(
    _In_ int argc,
    _In_reads_(argc) _Pre_z_ wchar_t** argv)
{
    ULONG Flags = WS_BORDER | WS_ACTIVECAPTION | WS_DLGFRAME | WS_MINIMIZEBOX | 3;

    W32ERROR Ret = UI_ValueEditorDlg(NULL,
                                     UIValueEditorCombine,
                                     &Flags,
                                     sizeof(Flags),
                                     g_Consts,
                                     ARRAYSIZE(g_Consts));
    return 0;
}

#endif
