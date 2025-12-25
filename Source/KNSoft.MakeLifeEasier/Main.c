#include "MakeLifeEasier.inl"

#pragma region I18N Support

static KNS_I18N_TABLE g_I18NTable = { PS_RUNONCE_INIT, &Precomp4C_I18N_Table_All };

PCWSTR
Mlep_GetStringEx(
    _In_ INT Index)
{
    return KNS_I18NGetString(&g_I18NTable, Index);
}

#pragma endregion

#pragma region Dialog Box Support

HRESULT
Mlep_DlgBox(
    _In_opt_ HWND Owner,
    _In_ PCWSTR DlgResName,
    _In_opt_ DLGPROC DlgProc,
    _In_opt_ LPARAM InitParam)
{
    HRESULT hr;
    PVOID DlgRes;
    DPI_AWARENESS_CONTEXT DPIContext;

    /* Access resource embedded by KNSoft.Precomp4C */
    if (!Precomp4C_Res2C_AccessResource(Precomp4C_Res2C_Resource_Embedded,
                                        ARRAYSIZE(Precomp4C_Res2C_Resource_Embedded),
                                        MAKEINTRESOURCEW(RT_DIALOG),
                                        DlgResName,
                                        LOCALE_NEUTRAL,
                                        &DlgRes,
                                        NULL))
    {
        return ERROR_RESOURCE_DATA_NOT_FOUND;
    }

    /* All KNSoft dialog boxes are DPI awared */
    DPIContext = UI_EnableDPIAwareContext();
    hr = KNS_OpenModelDialogBox((HINSTANCE)&__ImageBase, Owner, DlgRes, DlgProc, InitParam);
    UI_RestoreDPIAwareContext(DPIContext);
    return hr;
}

#pragma endregion

/* Test code */

#ifdef MSBUILD_CONFIGURATION_TYPE_EXE

int
_cdecl
wmain(
    _In_ int argc,
    _In_reads_(argc) _Pre_z_ wchar_t** argv)
{
    return 0;
}

#endif
