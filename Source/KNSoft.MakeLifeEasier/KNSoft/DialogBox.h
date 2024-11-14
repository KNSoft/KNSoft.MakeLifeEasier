#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

 /* All KNSoft dialog boxes return HRESULT */
FORCEINLINE
HRESULT
KNS_DlgBox(
    _In_opt_ HINSTANCE Instance,
    _In_ LPCDLGTEMPLATEW DialogTemplate,
    _In_opt_ HWND Owner,
    _In_opt_ DLGPROC DlgProc,
    _In_opt_ LPARAM InitParam)
{
    INT_PTR DlgRet;

    DlgRet = DialogBoxIndirectParamW(Instance, DialogTemplate, Owner, DlgProc, InitParam);

    return DlgRet != -1 ? (HRESULT)DlgRet : HRESULT_FROM_WIN32(NtGetLastError());
}

EXTERN_C_END
