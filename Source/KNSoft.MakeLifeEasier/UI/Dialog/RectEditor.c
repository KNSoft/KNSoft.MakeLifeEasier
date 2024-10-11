#include "../../MakeLifeEasier.inl"

#define MAX_INT32_IN_DEC_CCH 16

typedef struct _UI_RECTEDITOR_DATA
{
    PRECT       Rect;
    W32ERROR    Ret;
} UI_RECTEDITOR_DATA, *PUI_RECTEDITOR_DATA;

static
_Success_(return != FALSE)
LOGICAL
Dlg_RectEditor_GetValue(
    _In_ HWND Edit,
    _Out_ PINT Value)
{
    WCHAR Num[MAX_INT32_IN_DEC_CCH];

    return UI_GetWindowTextW(Edit, Num) && Str_ToIntW(Num, Value);
}

static
VOID
Dlg_RectEditor_SetValue(
    _In_ HWND Edit,
    _In_ INT Value)
{
    WCHAR Num[MAX_INT32_IN_DEC_CCH];

    UI_SetWindowTextW(Edit, Str_DecFromIntW(Value, Num) ? Num : NULL);
}

static
INT_PTR
CALLBACK
DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UI_DPIScaleDlgProc(hDlg, uMsg, wParam, lParam);
    if (uMsg == WM_INITDIALOG)
    {
        HWND hEdit;
        PUI_RECTEDITOR_DATA pData = (PUI_RECTEDITOR_DATA)lParam;

        UI_InitializeDialogDPIScale(hDlg);
        SetWindowLongPtrW(hDlg, DWLP_USER, lParam);
        UI_SetWindowTextW(hDlg, Mlep_GetString(Precomp4C_I18N_All_RectangleEditor));

        hEdit = GetDlgItem(hDlg, IDC_RECTEDITOR_TOP_EDIT);
        SendMessageW(hEdit, EM_SETLIMITTEXT, MAX_INT32_IN_DEC_CCH - 1, 0);
        Dlg_RectEditor_SetValue(hEdit, pData->Rect->top);
        UI_SetDlgItemTextW(hDlg, IDC_RECTEDITOR_TOP_TEXT, Mlep_GetString(Precomp4C_I18N_All_Top));

        hEdit = GetDlgItem(hDlg, IDC_RECTEDITOR_RIGHT_EDIT);
        SendMessageW(hEdit, EM_SETLIMITTEXT, MAX_INT32_IN_DEC_CCH - 1, 0);
        Dlg_RectEditor_SetValue(hEdit, pData->Rect->right);
        UI_SetDlgItemTextW(hDlg, IDC_RECTEDITOR_RIGHT_TEXT, Mlep_GetString(Precomp4C_I18N_All_Right));

        hEdit = GetDlgItem(hDlg, IDC_RECTEDITOR_BOTTOM_EDIT);
        SendMessageW(hEdit, EM_SETLIMITTEXT, MAX_INT32_IN_DEC_CCH - 1, 0);
        Dlg_RectEditor_SetValue(hEdit, pData->Rect->bottom);
        UI_SetDlgItemTextW(hDlg, IDC_RECTEDITOR_BOTTOM_TEXT, Mlep_GetString(Precomp4C_I18N_All_Bottom));

        hEdit = GetDlgItem(hDlg, IDC_RECTEDITOR_LEFT_EDIT);
        SendMessageW(hEdit, EM_SETLIMITTEXT, MAX_INT32_IN_DEC_CCH - 1, 0);
        Dlg_RectEditor_SetValue(hEdit, pData->Rect->left);
        UI_SetDlgItemTextW(hDlg, IDC_RECTEDITOR_LEFT_TEXT, Mlep_GetString(Precomp4C_I18N_All_Left));

        UI_SetDlgItemTextW(hDlg, IDOK, Mlep_GetString(Precomp4C_I18N_All_OK));
        UI_SetDlgItemTextW(hDlg, IDRETRY, Mlep_GetString(Precomp4C_I18N_All_Reset));

        return FALSE;
    } else if (uMsg == WM_COMMAND)
    {
        if (wParam == MAKEWPARAM(IDRETRY, 0))
        {
            PUI_RECTEDITOR_DATA pData = (PUI_RECTEDITOR_DATA)GetWindowLongPtrW(hDlg, DWLP_USER);

            Dlg_RectEditor_SetValue(GetDlgItem(hDlg, IDC_RECTEDITOR_TOP_EDIT), pData->Rect->top);
            Dlg_RectEditor_SetValue(GetDlgItem(hDlg, IDC_RECTEDITOR_RIGHT_EDIT), pData->Rect->right);
            Dlg_RectEditor_SetValue(GetDlgItem(hDlg, IDC_RECTEDITOR_BOTTOM_EDIT), pData->Rect->bottom);
            Dlg_RectEditor_SetValue(GetDlgItem(hDlg, IDC_RECTEDITOR_LEFT_EDIT), pData->Rect->left);
        } else if (wParam == MAKEWPARAM(IDOK, 0))
        {
            PUI_RECTEDITOR_DATA pData = (PUI_RECTEDITOR_DATA)GetWindowLongPtrW(hDlg, DWLP_USER);
            RECT rc;

            // Verify inputs
            if (Dlg_RectEditor_GetValue(GetDlgItem(hDlg, IDC_RECTEDITOR_LEFT_EDIT), &rc.left) &&
                Dlg_RectEditor_GetValue(GetDlgItem(hDlg, IDC_RECTEDITOR_TOP_EDIT), &rc.top) &&
                Dlg_RectEditor_GetValue(GetDlgItem(hDlg, IDC_RECTEDITOR_RIGHT_EDIT), &rc.right) &&
                Dlg_RectEditor_GetValue(GetDlgItem(hDlg, IDC_RECTEDITOR_BOTTOM_EDIT), &rc.bottom) &&
                rc.left <= rc.right && rc.top <= rc.bottom)
            {
                pData->Rect->left = rc.left;
                pData->Rect->top = rc.top;
                pData->Rect->right = rc.right;
                pData->Rect->bottom = rc.bottom;
                pData->Ret = ERROR_SUCCESS;
                EndDialog(hDlg, 0);
            } else
            {
                Error_Win32ErrorMessageBox(hDlg,
                                           Mlep_GetString(Precomp4C_I18N_All_RectangleEditor),
                                           ERROR_INVALID_PARAMETER);
            }
        }
        SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, 0);
    } else if (uMsg == WM_CLOSE)
    {
        PUI_RECTEDITOR_DATA pData = (PUI_RECTEDITOR_DATA)GetWindowLongPtrW(hDlg, DWLP_USER);

        pData->Ret = ERROR_CANCELLED;
        EndDialog(hDlg, 0);
        SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, 0);
    } else if (uMsg == WM_DESTROY)
    {
        UI_UninitializeDialogDPIScale(hDlg);
    }
    return 0;
}

W32ERROR
NTAPI
UI_RectEditorDlg(
    _In_opt_ HWND Owner,
    _Inout_ PRECT Rect)
{
    W32ERROR Ret;
    UI_RECTEDITOR_DATA Data = {
        .Rect = Rect,
        .Ret = ERROR_INTERNAL_ERROR
    };

    Ret = Mlep_DlgBox(MAKEINTRESOURCEW(IDD_RECTEDITOR), Owner, DlgProc, (LPARAM)&Data, NULL);
    return Ret == ERROR_SUCCESS ? Data.Ret : Ret;
}
