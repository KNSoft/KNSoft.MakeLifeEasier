#include "../../MakeLifeEasier.inl"

#define UI_CAPTURE_WINDOW L"KNSoft.MakeLifeEasier.UI.CaptureWindow"
#define UI_CAPTURE_WINDOW_BORDER -2

typedef struct _UI_CAPTURE_WINDOW_DATA
{
    /* Input */
    UI_SNAPSHOT Snapshot;
    HWND TargetWindow;
    struct
    {
        ULONG IgnoreChild : 1;
        ULONG IgnoreTransparent : 1;
    };
    HCURSOR Cursor;

    /* Internal use */
    HWND CaptureWindow;
    HWND TempTarget;
    POINT Position;
    LOGICAL Done;
    HRESULT Result;
} UI_CAPTURE_WINDOW_DATA, *PUI_CAPTURE_WINDOW_DATA;

static
BOOL
CALLBACK
CaptureWndEnumProc(
    HWND hWnd,
    LPARAM lParam)
{
    RECT rc;
    PUI_CAPTURE_WINDOW_DATA Data = (PUI_CAPTURE_WINDOW_DATA)lParam;

    if (hWnd != Data->CaptureWindow &&
        IsWindowVisible(hWnd) &&
        (!Data->IgnoreTransparent || !(GetWindowLongPtrW(hWnd, GWL_EXSTYLE) & WS_EX_TRANSPARENT)) &&
        !IsIconic(hWnd) &&
        UI_GetWindowCloackedState(hWnd) == 0 &&
        SUCCEEDED(UI_GetWindowRect(hWnd, &rc)) &&
        UI_PtInRect(&rc, &Data->Position))
    {
        Data->TempTarget = hWnd;
        return FALSE;
    }
    return TRUE;
}

static BLENDFUNCTION g_BlendFunc = { AC_SRC_OVER, 0, 128, 0 };

static
LRESULT
CALLBACK
CaptureWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_CREATE)
    {
        LPCREATESTRUCTW pCreate = (LPCREATESTRUCTW)lParam;
        if (pCreate->lpCreateParams == NULL ||
            UI_SetWindowLong(hWnd, GWLP_USERDATA, (LONG_PTR)pCreate->lpCreateParams) != ERROR_SUCCESS)
        {
            return -1;
        }
        ((PUI_CAPTURE_WINDOW_DATA)pCreate->lpCreateParams)->CaptureWindow = hWnd;
    } else if (uMsg == WM_SETCURSOR)
    {
        if (LOWORD(lParam) == HTCLIENT)
        {
            PUI_CAPTURE_WINDOW_DATA Data = (PUI_CAPTURE_WINDOW_DATA)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
            SetCursor(Data->Cursor);
            return TRUE;
        }
    } else if (uMsg == WM_PAINT)
    {
        PUI_CAPTURE_WINDOW_DATA Data = (PUI_CAPTURE_WINDOW_DATA)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
        UI_BUFFEREDPAINT Paint;
        RECT rc;

        if (!UI_BeginBufferedPaint(hWnd, &Paint))
        {
            return 0;
        }
        GdiAlphaBlend(Paint.hdc,
                      0,
                      0,
                      Data->Snapshot.Size.cx,
                      Data->Snapshot.Size.cy,
                      Data->Snapshot.DC,
                      0,
                      0,
                      Data->Snapshot.Size.cx,
                      Data->Snapshot.Size.cy,
                      g_BlendFunc);
        if (Data->TargetWindow != NULL &&
            SUCCEEDED(UI_GetRelativeRect(Data->TargetWindow, hWnd, &rc)))
        {
            BitBlt(Paint.hdc,
                   rc.left,
                   rc.top,
                   rc.right - rc.left,
                   rc.bottom - rc.top,
                   Data->Snapshot.DC,
                   rc.left,
                   rc.top,
                   SRCCOPY);
            UI_DrawFrameRect(Paint.hdc, &rc, UI_CAPTURE_WINDOW_BORDER, DSTINVERT);
        }
        UI_EndBufferedPaint(hWnd, &Paint);
        return 0;
    } else if (uMsg == WM_MOUSEMOVE)
    {
        PUI_CAPTURE_WINDOW_DATA Data = (PUI_CAPTURE_WINDOW_DATA)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
        HWND hWndParent;
        POINT pt;

        Data->Position.x = GET_X_LPARAM(lParam);
        Data->Position.y = GET_Y_LPARAM(lParam);
        if (ClientToScreen(hWnd, &Data->Position))
        {
            hWndParent = GetDesktopWindow();
            Data->TempTarget = NULL;
            if (!Data->IgnoreChild)
            {
                while (TRUE)
                {
                    pt = Data->Position;
                    if (!ScreenToClient(hWndParent, &pt))
                    {
                        break;
                    }
                    UI_EnumChildWindows(hWndParent, CaptureWndEnumProc, (LPARAM)Data);
                    if (Data->TempTarget == NULL)
                    {
                        Data->TempTarget = hWndParent;
                        break;
                    } else if (Data->TempTarget == hWndParent)
                    {
                        break;
                    } else
                    {
                        hWndParent = Data->TempTarget;
                        Data->TempTarget = NULL;
                    }
                }
            } else
            {
                UI_EnumChildWindows(hWndParent, CaptureWndEnumProc, (LPARAM)Data);
            }
            if (Data->TargetWindow != Data->TempTarget)
            {
                Data->TargetWindow = Data->TempTarget;
                UI_Redraw(hWnd);
            }
        }
        return 0;
    } else if (uMsg == WM_LBUTTONUP)
    {
        PUI_CAPTURE_WINDOW_DATA Data = (PUI_CAPTURE_WINDOW_DATA)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
        Data->Result = S_OK;
        DestroyWindow(hWnd);
        return 0;
    } else if (uMsg == WM_KEYUP)
    {
        PUI_CAPTURE_WINDOW_DATA Data = (PUI_CAPTURE_WINDOW_DATA)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
        if (wParam == VK_CANCEL || wParam == VK_ESCAPE || wParam == VK_END || wParam == VK_RETURN)
        {
            if (wParam != VK_RETURN)
            {
                Data->TargetWindow = NULL;
                Data->Result = S_FALSE;
                Data->Done = TRUE;
            }
            if (wParam == VK_RETURN)
            {
                Data->Result = S_OK;
            }
            DestroyWindow(hWnd);
        }
        return 0;
    } else if (uMsg == WM_DESTROY)
    {
        PUI_CAPTURE_WINDOW_DATA Data = (PUI_CAPTURE_WINDOW_DATA)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
        if (Data != NULL && !Data->Done)
        {
            Data->Done = TRUE;
        }
        return 0;
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

static CONST WNDCLASSEXW g_Class = {
        sizeof(g_Class),
        0,
        CaptureWndProc,
        0,
        0,
        (HINSTANCE)&__ImageBase,
        NULL,
        NULL,
        NULL,
        NULL,
        UI_CAPTURE_WINDOW,
        NULL
};

HRESULT
NTAPI
UI_WindowPickerDlg(
    _In_ LOGICAL IgnoreChild,
    _In_ LOGICAL IgnoreTransparent,
    _In_opt_ HCURSOR Cursor,
    _Out_ HWND * TargetWindow)
{
    HRESULT hr;
    PUI_CAPTURE_WINDOW_DATA Data;
    DPI_AWARENESS_CONTEXT DPIContext;
    ATOM atom;
    HWND hWnd;
    BOOL Ret;
    MSG Msg;

    if (!Mem_AllocPtr(Data))
    {
        return E_OUTOFMEMORY;
    }
    DPIContext = UI_EnableDPIAwareContext();
    if (!UI_CreateSnapshot(NULL, &Data->Snapshot))
    {
        hr = E_UNEXPECTED;
        goto _Exit_0;
    }
    atom = RegisterClassExW(&g_Class);
    if (atom == 0)
    {
        hr = HRESULT_FROM_WIN32(Err_GetLastError());
        goto _Exit_1;
    }
    Data->Cursor = Cursor == NULL ? LoadImageW(NULL,
                                               MAKEINTRESOURCEW(OCR_NORMAL),
                                               IMAGE_CURSOR,
                                               0,
                                               0,
                                               LR_DEFAULTSIZE | LR_SHARED) : Cursor;
    Data->TargetWindow = NULL;
    Data->IgnoreChild = IgnoreChild;
    Data->IgnoreTransparent = IgnoreTransparent;
    Data->Done = FALSE;
    Data->Result = S_FALSE;
    hWnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                           MAKEINTRESOURCEW(atom),
                           UI_CAPTURE_WINDOW,
                           WS_POPUP | WS_VISIBLE,
                           Data->Snapshot.Position.x,
                           Data->Snapshot.Position.y,
                           Data->Snapshot.Size.cx,
                           Data->Snapshot.Size.cy,
                           HWND_DESKTOP,
                           NULL,
                           (HINSTANCE)&__ImageBase,
                           Data);
    if (hWnd == NULL)
    {
        hr = HRESULT_FROM_WIN32(Err_GetLastError());
        goto _Exit_2;
    }
    while (!Data->Done)
    {
        Ret = GetMessageW(&Msg, hWnd, 0, 0);
        if (Ret == -1)
        {
            hr = HRESULT_FROM_WIN32(Err_GetLastError());
            goto _Exit_2;
        } else if (Ret == 0)
        {
            PostQuitMessage((INT)Msg.wParam);
            hr = E_ABORT;
            goto _Exit_2;
        }
        TranslateMessage(&Msg);
        DispatchMessageW(&Msg);
    }
    hr = Data->Result;
    if (SUCCEEDED(hr))
    {
        *TargetWindow = Data->TargetWindow;
    }

_Exit_2:
    UnregisterClassW(MAKEINTRESOURCEW(atom), (HINSTANCE)&__ImageBase);
_Exit_1:
    UI_DeleteSnapshot(&Data->Snapshot);
_Exit_0:
    UI_RestoreDPIAwareContext(DPIContext);
    Mem_Free(Data);
    return hr;
}
