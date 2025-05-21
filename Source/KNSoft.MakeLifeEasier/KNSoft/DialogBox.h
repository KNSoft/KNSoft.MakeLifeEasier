#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

 /* All KNSoft dialog boxes return HRESULT */

FORCEINLINE
HRESULT
KNS_DlgMessageLoop(
    _In_opt_ HACCEL Accelerator)
{
    INT DlgRet;
    W32ERROR Ret;

    Ret = UI_MessageLoop(NULL, TRUE, Accelerator, &DlgRet);
    return Ret == ERROR_SUCCESS ? (HRESULT)DlgRet : HRESULT_FROM_WIN32(Ret);
}

FORCEINLINE
HRESULT
KNS_OpenModelDialogBox(
    _In_opt_ HINSTANCE Instance,
    _In_opt_ HWND Owner,
    _In_ LPCDLGTEMPLATEW DialogTemplate,
    _In_opt_ DLGPROC DlgProc,
    _In_opt_ LPARAM InitParam)
{
    INT_PTR DlgRet;

    DlgRet = DialogBoxIndirectParamW(Instance, DialogTemplate, Owner, DlgProc, InitParam);
    return DlgRet != -1 ? (HRESULT)DlgRet : HRESULT_FROM_WIN32(Err_GetLastError());
}

EXTERN_C_END
