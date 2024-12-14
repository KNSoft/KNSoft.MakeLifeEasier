#include "../../MakeLifeEasier.inl"

#define MAX_INT32_IN_DEC_CCH 16

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

    UI_SetWindowTextW(Edit, Str_DecFromIntW(Value, Num) > 0 ? Num : NULL);
}

static
INT_PTR
CALLBACK
DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UI_DPIScaleDlgProc(hDlg, uMsg, wParam, lParam);
    if (uMsg == WM_INITDIALOG)
    {
        PRECT prc = (PRECT)lParam;
        HWND hEdit;

        SetWindowLongPtrW(hDlg, DWLP_USER, lParam);
        UI_SetWindowTextW(hDlg, Mlep_GetString(RectangleEditor));

        hEdit = GetDlgItem(hDlg, IDC_RECTEDITOR_TOP_EDIT);
        SendMessageW(hEdit, EM_SETLIMITTEXT, MAX_INT32_IN_DEC_CCH - 1, 0);
        Dlg_RectEditor_SetValue(hEdit, prc->top);
        UI_SetDlgItemTextW(hDlg, IDC_RECTEDITOR_TOP_TEXT, Mlep_GetString(Top));

        hEdit = GetDlgItem(hDlg, IDC_RECTEDITOR_RIGHT_EDIT);
        SendMessageW(hEdit, EM_SETLIMITTEXT, MAX_INT32_IN_DEC_CCH - 1, 0);
        Dlg_RectEditor_SetValue(hEdit, prc->right);
        UI_SetDlgItemTextW(hDlg, IDC_RECTEDITOR_RIGHT_TEXT, Mlep_GetString(Right));

        hEdit = GetDlgItem(hDlg, IDC_RECTEDITOR_BOTTOM_EDIT);
        SendMessageW(hEdit, EM_SETLIMITTEXT, MAX_INT32_IN_DEC_CCH - 1, 0);
        Dlg_RectEditor_SetValue(hEdit, prc->bottom);
        UI_SetDlgItemTextW(hDlg, IDC_RECTEDITOR_BOTTOM_TEXT, Mlep_GetString(Bottom));

        hEdit = GetDlgItem(hDlg, IDC_RECTEDITOR_LEFT_EDIT);
        SendMessageW(hEdit, EM_SETLIMITTEXT, MAX_INT32_IN_DEC_CCH - 1, 0);
        Dlg_RectEditor_SetValue(hEdit, prc->left);
        UI_SetDlgItemTextW(hDlg, IDC_RECTEDITOR_LEFT_TEXT, Mlep_GetString(Left));

        UI_SetDlgItemTextW(hDlg, IDOK, Mlep_GetString(OK));
        UI_SetDlgItemTextW(hDlg, IDRETRY, Mlep_GetString(Reset));

        return TRUE;
    } else if (uMsg == WM_COMMAND)
    {
        if (wParam == MAKEWPARAM(IDRETRY, 0))
        {
            PRECT prc = (PRECT)GetWindowLongPtrW(hDlg, DWLP_USER);

            Dlg_RectEditor_SetValue(GetDlgItem(hDlg, IDC_RECTEDITOR_TOP_EDIT), prc->top);
            Dlg_RectEditor_SetValue(GetDlgItem(hDlg, IDC_RECTEDITOR_RIGHT_EDIT), prc->right);
            Dlg_RectEditor_SetValue(GetDlgItem(hDlg, IDC_RECTEDITOR_BOTTOM_EDIT), prc->bottom);
            Dlg_RectEditor_SetValue(GetDlgItem(hDlg, IDC_RECTEDITOR_LEFT_EDIT), prc->left);
        } else if (wParam == MAKEWPARAM(IDOK, 0))
        {
            PRECT prc = (PRECT)GetWindowLongPtrW(hDlg, DWLP_USER);
            RECT rc;

            // Verify inputs
            if (Dlg_RectEditor_GetValue(GetDlgItem(hDlg, IDC_RECTEDITOR_LEFT_EDIT), &rc.left) &&
                Dlg_RectEditor_GetValue(GetDlgItem(hDlg, IDC_RECTEDITOR_TOP_EDIT), &rc.top) &&
                Dlg_RectEditor_GetValue(GetDlgItem(hDlg, IDC_RECTEDITOR_RIGHT_EDIT), &rc.right) &&
                Dlg_RectEditor_GetValue(GetDlgItem(hDlg, IDC_RECTEDITOR_BOTTOM_EDIT), &rc.bottom) &&
                rc.left <= rc.right && rc.top <= rc.bottom)
            {
                prc->left = rc.left;
                prc->top = rc.top;
                prc->right = rc.right;
                prc->bottom = rc.bottom;
                EndDialog(hDlg, S_OK);
            } else
            {
                Err_Win32ErrorMessageBox(hDlg,
                                         Mlep_GetString(RectangleEditor),
                                         ERROR_INVALID_PARAMETER);
            }
        }
        SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, 0);
    } else if (uMsg == WM_CLOSE)
    {
        PRECT prc = (PRECT)GetWindowLongPtrW(hDlg, DWLP_USER);

        EndDialog(hDlg, S_FALSE);
        SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, 0);
    }
    return 0;
}

HRESULT
NTAPI
UI_RectEditorDlg(
    _In_opt_ HWND Owner,
    _Inout_ PRECT Rect)
{
    return Mlep_DlgBox(Owner, MAKEINTRESOURCEW(IDD_RECTEDITOR), DlgProc, (LPARAM)Rect);
}
