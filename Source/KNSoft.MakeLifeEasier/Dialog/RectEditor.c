#include "../MakeLifeEasier.inl"

#define DRE_MAX_NUM_CCH 16

typedef struct _DLG_RECTEDITOR
{
    PRECT       Rect;
    W32ERROR    Ret;
} DLG_RECTEDITOR, *PDLG_RECTEDITOR;

static
_Success_(return != FALSE)
LOGICAL
Dlg_RectEditor_GetValue(
    _In_ HWND Edit,
    _Out_ PINT Value)
{
    WCHAR Num[DRE_MAX_NUM_CCH];

    return UI_GetWindowTextW(Edit, Num) && Str_ToIntW(Num, Value);
}

static
VOID
Dlg_RectEditor_SetValue(
    _In_ HWND Edit,
    _In_ INT Value)
{
    WCHAR Num[DRE_MAX_NUM_CCH];

    UI_SetWindowTextW(Edit, Str_DecFromIntW(Value, Num) ? Num : NULL);
}

static
INT_PTR
CALLBACK
DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG)
    {
        HWND hEdit;
        PDLG_RECTEDITOR lpstDRE = (PDLG_RECTEDITOR)lParam;

        SetWindowLongPtrW(hDlg, DWLP_USER, lParam);
        UI_SetWindowTextW(hDlg, Mlep_GetString(I18N_All_RectangleEditor));

        hEdit = GetDlgItem(hDlg, IDC_RECTEDITOR_TOP_EDIT);
        SendMessageW(hEdit, EM_SETLIMITTEXT, DRE_MAX_NUM_CCH - 1, 0);
        Dlg_RectEditor_SetValue(hEdit, lpstDRE->Rect->top);
        UI_SetDlgItemTextW(hDlg, IDC_RECTEDITOR_TOP_TEXT, Mlep_GetString(I18N_All_Top));

        hEdit = GetDlgItem(hDlg, IDC_RECTEDITOR_RIGHT_EDIT);
        SendMessageW(hEdit, EM_SETLIMITTEXT, DRE_MAX_NUM_CCH - 1, 0);
        Dlg_RectEditor_SetValue(hEdit, lpstDRE->Rect->right);
        UI_SetDlgItemTextW(hDlg, IDC_RECTEDITOR_RIGHT_TEXT, Mlep_GetString(I18N_All_Right));

        hEdit = GetDlgItem(hDlg, IDC_RECTEDITOR_BOTTOM_EDIT);
        SendMessageW(hEdit, EM_SETLIMITTEXT, DRE_MAX_NUM_CCH - 1, 0);
        Dlg_RectEditor_SetValue(hEdit, lpstDRE->Rect->bottom);
        UI_SetDlgItemTextW(hDlg, IDC_RECTEDITOR_BOTTOM_TEXT, Mlep_GetString(I18N_All_Bottom));

        hEdit = GetDlgItem(hDlg, IDC_RECTEDITOR_LEFT_EDIT);
        SendMessageW(hEdit, EM_SETLIMITTEXT, DRE_MAX_NUM_CCH - 1, 0);
        Dlg_RectEditor_SetValue(hEdit, lpstDRE->Rect->left);
        UI_SetDlgItemTextW(hDlg, IDC_RECTEDITOR_LEFT_TEXT, Mlep_GetString(I18N_All_Left));

        UI_SetDlgItemTextW(hDlg, IDOK, Mlep_GetString(I18N_All_OK));
        UI_SetDlgItemTextW(hDlg, IDRETRY, Mlep_GetString(I18N_All_Reset));

        return FALSE;

    } else if (uMsg == WM_COMMAND)
    {
        if (wParam == MAKEWPARAM(IDRETRY, 0))
        {
            PDLG_RECTEDITOR lpstDRE = (PDLG_RECTEDITOR)GetWindowLongPtrW(hDlg, DWLP_USER);
            Dlg_RectEditor_SetValue(GetDlgItem(hDlg, IDC_RECTEDITOR_TOP_EDIT), lpstDRE->Rect->top);
            Dlg_RectEditor_SetValue(GetDlgItem(hDlg, IDC_RECTEDITOR_RIGHT_EDIT), lpstDRE->Rect->right);
            Dlg_RectEditor_SetValue(GetDlgItem(hDlg, IDC_RECTEDITOR_BOTTOM_EDIT), lpstDRE->Rect->bottom);
            Dlg_RectEditor_SetValue(GetDlgItem(hDlg, IDC_RECTEDITOR_LEFT_EDIT), lpstDRE->Rect->left);
        } else if (wParam == MAKEWPARAM(IDOK, 0))
        {
            PDLG_RECTEDITOR lpstDRE = (PDLG_RECTEDITOR)GetWindowLongPtrW(hDlg, DWLP_USER);
            RECT rc;

            // Verify inputs
            if (Dlg_RectEditor_GetValue(GetDlgItem(hDlg, IDC_RECTEDITOR_LEFT_EDIT), &rc.left) &&
                Dlg_RectEditor_GetValue(GetDlgItem(hDlg, IDC_RECTEDITOR_TOP_EDIT), &rc.top) &&
                Dlg_RectEditor_GetValue(GetDlgItem(hDlg, IDC_RECTEDITOR_RIGHT_EDIT), &rc.right) &&
                Dlg_RectEditor_GetValue(GetDlgItem(hDlg, IDC_RECTEDITOR_BOTTOM_EDIT), &rc.bottom) &&
                rc.left <= rc.right && rc.top <= rc.bottom)
            {
                lpstDRE->Rect->left = rc.left;
                lpstDRE->Rect->top = rc.top;
                lpstDRE->Rect->right = rc.right;
                lpstDRE->Rect->bottom = rc.bottom;
                lpstDRE->Ret = ERROR_SUCCESS;
                EndDialog(hDlg, 0);
            } else
            {
                Error_Win32ErrorMessageBox(hDlg, Mlep_GetString(I18N_All_RectangleEditor), ERROR_INVALID_PARAMETER);
            }
        }
        SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, 0);
    } else if (uMsg == WM_CLOSE)
    {
        PDLG_RECTEDITOR lpstDRE = (PDLG_RECTEDITOR)GetWindowLongPtrW(hDlg, DWLP_USER);

        lpstDRE->Ret = ERROR_CANCELLED;
        EndDialog(hDlg, 0);
        SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, 0);
    }
    return 0;
}

W32ERROR
NTAPI
Dlg_RectEditor(
    _In_opt_ HWND Owner,
    _Inout_ PRECT Rect)
{
    NTSTATUS Status;
    PVOID Res;
    ULONG Len;
    DLG_RECTEDITOR Info = {
        .Rect = Rect,
        .Ret = ERROR_INTERNAL_ERROR
    };

    Status = Mlep_AccessResource(RT_DIALOG, MAKEINTRESOURCEW(IDD_RECTEDITOR), LANG_USER_DEFAULT, &Res, &Len);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    return DialogBoxIndirectParamW((HINSTANCE)&__ImageBase, Res, Owner, DlgProc, (LPARAM)&Info) == -1 ?
        NtGetLastError() : Info.Ret;
}
