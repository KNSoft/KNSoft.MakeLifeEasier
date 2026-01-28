#include "../MakeLifeEasier.inl"

HRESULT
NTAPI
UI_GetRelativeRect(
    _In_ HWND Window,
    _In_opt_ HWND RefWindow,
    _Out_ PRECT Rect)
{
    HANDLE hParent;
    RECT rc;
    HRESULT hr;

    hr = UI_GetWindowRect(Window, &rc);
    if (FAILED(hr))
    {
        return hr;
    }

    hParent = RefWindow;
    if (hParent == NULL)
    {
        hParent = GetAncestor(Window, GA_PARENT);
        if (hParent == NULL)
        {
            hParent = GetDesktopWindow();
        }
    }
    return UI_ScreenRectToClient(hParent, &rc, Rect) ? S_OK : E_UNEXPECTED;
}

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

LOGICAL
NTAPI
UI_SetWindowThemeProperty(
    _In_ ATOM AtomTable,
    _In_ HWND Window,
    _In_opt_ PCWSTR Property)
{
    ATOM Prop;
    LOGICAL Ret;

    /* Remove the previously applied associations if Property is NULL */
    if (Property == NULL)
    {
        Prop = (ATOM)RemovePropW(Window, (PCWSTR)AtomTable);
        if (Prop != INVALID_ATOM)
        {
            DeleteAtom(Prop);
        }
        return TRUE;
    }

    /* Disable visual style if Property is an empty string */
    Prop = AddAtomW(Property[0] != UNICODE_NULL ? Property : L"$");
    if (Prop == INVALID_ATOM)
    {
        return FALSE;
    }
    Ret = SetPropW(Window, (PCWSTR)AtomTable, (HANDLE)Prop);
    if (!Ret)
    {
        DeleteAtom(Prop);
    }
    return Ret;
}

static
_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS
NTAPI
UI_FlashWindow_Thread(
    _In_ PVOID ThreadParameter)
{
    NTSTATUS Status;
    HWND Window;
    HDC ScreenDC;
    RECT Rect;
    UINT Times;

    Window = (HWND)ThreadParameter;
    Times = 4 * 2;
    ScreenDC = GetDC(NULL);
    if (ScreenDC == NULL)
    {
        return STATUS_INVALID_HANDLE;
    }
    if (SUCCEEDED(UI_GetWindowRect(Window, &Rect)))
    {
        do
        {
            UI_DrawFrameRect(ScreenDC, &Rect, -3, PATINVERT);
            PS_DelayExec(100);
            Times--;
        } while (Times != 0);
        Status = STATUS_SUCCESS;
    } else
    {
        Status = NTSTATUS_FROM_WIN32(Err_GetLastError());
    }
    ReleaseDC(NULL, ScreenDC);
    RedrawWindow(NULL, &Rect, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);

    return Status;
}

LOGICAL
NTAPI
UI_FlashWindow(
    _In_ HWND Window)
{
    return NT_SUCCESS(PS_CreateThread(NtCurrentProcess(),
                                      FALSE,
                                      UI_FlashWindow_Thread,
                                      (PVOID)Window,
                                      NULL,
                                      NULL));
}
