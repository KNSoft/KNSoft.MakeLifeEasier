#include "../MakeLifeEasier.inl"

static
VOID
UI_UpdateResizeWndDPI(
    _In_ HWND Window,
    _In_ UINT NewDPIX,
    _In_ UINT NewDPIY,
    _Inout_ PUI_WINDOW_RESIZE_INFO pInfo)
{
    UI_DPIScaleRect(&pInfo->Rect, pInfo->DPIX, NewDPIX, pInfo->DPIY, NewDPIY);
    UI_DPIAdjustWindowRect(Window, &pInfo->Rect, NewDPIX);
    pInfo->Rect.right -= pInfo->Rect.left;
    pInfo->Rect.bottom -= pInfo->Rect.top;
    pInfo->Rect.top = pInfo->Rect.left = 0;
    pInfo->DPIX = NewDPIX;
    pInfo->DPIY = NewDPIY;
}

INT_PTR
CALLBACK
UI_ResizeWndProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    _In_ LOGICAL bDialog,
    _In_opt_ __callback PUI_WINDOW_RESIZE_FN pfnResizeCallback,
    _Inout_ PUI_WINDOW_RESIZE_INFO pInfo)
{
    if (uMsg == WM_GETMINMAXINFO)
    {
        PMINMAXINFO pMinMaxInfo = (PMINMAXINFO)lParam;
        UINT DPIX, DPIY;
        POINT ptOrigin;

        if (pInfo->Rect.right == 0 && pInfo->Rect.bottom == 0)
        {
            ptOrigin.x = ptOrigin.y = 0;
            if (GetClientRect(hWnd, &pInfo->Rect) && ClientToScreen(hWnd, &ptOrigin))
            {
                pInfo->Rect.left += ptOrigin.x;
                pInfo->Rect.right += ptOrigin.x;
                pInfo->Rect.top += ptOrigin.y;
                pInfo->Rect.bottom += ptOrigin.y;
                pInfo->DPIX = pInfo->DPIY = USER_DEFAULT_SCREEN_DPI;
                UI_GetWindowDPI(hWnd, &DPIX, &DPIY);
                UI_UpdateResizeWndDPI(hWnd, DPIX, DPIY, pInfo);
            }
        } else
        {
            pMinMaxInfo->ptMinTrackSize.x = pInfo->Rect.right;
            pMinMaxInfo->ptMinTrackSize.y = pInfo->Rect.bottom;
        }
    } else if (uMsg == WM_DPICHANGED)
    {
        UI_UpdateResizeWndDPI(hWnd, LOWORD(wParam), HIWORD(wParam), pInfo);
    } else if (uMsg == WM_SIZE)
    {
        if (pfnResizeCallback != NULL && (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED))
        {
            pfnResizeCallback(hWnd, LOWORD(lParam), HIWORD(lParam), NULL);
        }
    } else if (uMsg == WM_WINDOWPOSCHANGED)
    {
        PWINDOWPOS pWndPos = (PWINDOWPOS)lParam;
        RECT rc;

        if (!(pWndPos->flags & SWP_NOSIZE) && pfnResizeCallback != NULL && GetClientRect(hWnd, &rc))
        {
            pfnResizeCallback(hWnd, rc.right, rc.bottom, pWndPos);
        }
    } else
    {
        return 0;
    }

    if (bDialog)
    {
        SetWindowLongPtrW(hWnd, DWLP_MSGRESULT, 0);
    }
    return 0;
}
