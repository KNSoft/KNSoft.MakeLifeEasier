#include "../../MakeLifeEasier.inl"

INT_PTR
CALLBACK
UI_PropSheetWndProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    INT iTabControlId)
{
    if (uMsg == WM_NOTIFY)
    {
        LPNMHDR lpnmhdr = (LPNMHDR)lParam;
        INT iSelected;
        TCITEMW tci;
        RECT rc;
        if (lpnmhdr->idFrom == iTabControlId)
        {
            if (lpnmhdr->code == TCN_SELCHANGING || lpnmhdr->code == TCN_SELCHANGE)
            {
                iSelected = (INT)SendMessageW(lpnmhdr->hwndFrom, TCM_GETCURSEL, 0, 0);
                if (iSelected != -1)
                {
                    tci.mask = TCIF_PARAM;
                    if (SendMessageW(lpnmhdr->hwndFrom, TCM_GETITEMW, iSelected, (LPARAM)&tci))
                    {
                        if (lpnmhdr->code == TCN_SELCHANGING)
                        {
                            ShowWindow((HWND)tci.lParam, SW_HIDE);
                        } else
                        {
                            if (SUCCEEDED(UI_GetRelativeRect(lpnmhdr->hwndFrom, hWnd, &rc)))
                            {
                                SendMessageW(lpnmhdr->hwndFrom, TCM_ADJUSTRECT, FALSE, (LPARAM)&rc);
                                SetWindowPos((HWND)tci.lParam,
                                             NULL,
                                             rc.left,
                                             rc.top,
                                             rc.right - rc.left,
                                             rc.bottom - rc.top,
                                             SWP_NOZORDER | SWP_SHOWWINDOW);
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}

LOGICAL
NTAPI
UI_InitPropSheetEx(
    _In_ HWND Dialog,
    _In_ INT TabControlId,
    _In_reads_(PageCount) UI_PROPSHEET_PAGE Pages[],
    _In_ UINT PageCount)
{
    HWND TabControl;
    UINT i;
    TCITEMW tci;
    NMHDR nmhdr;

    TabControl = GetDlgItem(Dialog, TabControlId);
    tci.mask = TCIF_TEXT | TCIF_PARAM;
    SendMessageW(TabControl, TCM_DELETEALLITEMS, 0, 0);
    for (i = 0; i < PageCount; i++)
    {
        tci.pszText = (PWSTR)Pages[i].TabTitle;
        tci.lParam = (LPARAM)Pages[i].PageWindow;
        if (SendMessageW(TabControl, TCM_INSERTITEMW, i, (LPARAM)&tci) == -1)
        {
            SendMessageW(TabControl, TCM_DELETEALLITEMS, 0, 0);
            return FALSE;
        }
        UI_SetDialogTextureTheme(Pages[i].PageWindow, ETDT_ENABLETAB);
    }
    if (PageCount)
    {
        SetWindowPos(TabControl, Pages[i - 1].PageWindow, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
        SendMessageW(TabControl, TCM_SETCURSEL, 0, 0);
        nmhdr.hwndFrom = TabControl;
        nmhdr.idFrom = TabControlId;
        nmhdr.code = TCN_SELCHANGE;
        SendMessageW(Dialog, WM_NOTIFY, TabControlId, (LPARAM)&nmhdr);
    }

    return TRUE;
}
