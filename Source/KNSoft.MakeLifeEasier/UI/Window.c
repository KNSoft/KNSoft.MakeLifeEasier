﻿#include "../MakeLifeEasier.inl"

#define UI_WINDOW_RESIZE_PROP L"KNSoft.MakeLifeEasier.UI.WindowResize"

typedef struct _UI_WINDOW_RESIZE_INFO
{
    /* 0 if no limit */
    LONG MinWidth;
    LONG MinHeight;
    PUI_WINDOW_RESIZE_FN Callback;

    /* Internal data */
    UINT DPIX;
    UINT DPIY;
} UI_WINDOW_RESIZE_INFO, *PUI_WINDOW_RESIZE_INFO;

W32ERROR
NTAPI
UI_InitializeWindowResize(
    _In_ HWND Window,
    _In_opt_ LONG MinWidth,
    _In_opt_ LONG MinHeight,
    _In_opt_ PUI_WINDOW_RESIZE_FN Callback)
{
    PUI_WINDOW_RESIZE_INFO ResizeInfo;

    if (!Mem_AllocPtr(ResizeInfo))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    ResizeInfo->MinWidth = MinWidth;
    ResizeInfo->MinHeight = MinHeight;
    ResizeInfo->Callback = Callback;
    UI_GetWindowDPI(Window, &ResizeInfo->DPIX, &ResizeInfo->DPIY);
    return SetPropW(Window, UI_WINDOW_RESIZE_PROP, (HANDLE)ResizeInfo) ? ERROR_SUCCESS : NtGetLastError();
}

VOID
NTAPI
UI_UninitializeWindowResize(
    _In_ HWND Window)
{
    PUI_WINDOW_RESIZE_INFO ResizeInfo;

    ResizeInfo = GetPropW(Window, UI_WINDOW_RESIZE_PROP);
    if (ResizeInfo != NULL)
    {
        Mem_Free(ResizeInfo);
        RemovePropW(Window, UI_WINDOW_RESIZE_PROP);
    }
}

INT_PTR
CALLBACK
UI_ResizeWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_DPICHANGED)
    {
        PUI_WINDOW_RESIZE_INFO ResizeInfo = GetPropW(hWnd, UI_WINDOW_RESIZE_PROP);
        UINT DPIX = LOWORD(wParam), DPIY = HIWORD(wParam);

        UI_DPIScaleInt(&ResizeInfo->MinWidth, ResizeInfo->DPIX, DPIX);
        UI_DPIScaleInt(&ResizeInfo->MinHeight, ResizeInfo->DPIY, DPIY);
        ResizeInfo->DPIX = DPIX;
        ResizeInfo->DPIY = DPIY;
    } else if (uMsg == WM_SIZING)
    {
        PUI_WINDOW_RESIZE_INFO ResizeInfo = GetPropW(hWnd, UI_WINDOW_RESIZE_PROP);
        PRECT prc = (PRECT)lParam;

        if (ResizeInfo->MinWidth && prc->right < prc->left + ResizeInfo->MinWidth)
        {
            prc->right = prc->left + ResizeInfo->MinWidth;
        }
        if (ResizeInfo->MinHeight && prc->bottom < prc->top + ResizeInfo->MinHeight)
        {
            prc->bottom = prc->top + ResizeInfo->MinHeight;
        }
        return TRUE;
    } else if (uMsg == WM_SIZE)
    {
        PUI_WINDOW_RESIZE_INFO ResizeInfo = GetPropW(hWnd, UI_WINDOW_RESIZE_PROP);

        if (ResizeInfo->Callback != NULL && (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED))
        {
            ResizeInfo->Callback(hWnd, LOWORD(lParam), HIWORD(lParam), NULL);
        }
    } else if (uMsg == WM_WINDOWPOSCHANGED)
    {
        PUI_WINDOW_RESIZE_INFO ResizeInfo = GetPropW(hWnd, UI_WINDOW_RESIZE_PROP);
        RECT rc;

        if (ResizeInfo->Callback != NULL && GetClientRect(hWnd, &rc))
        {
            ResizeInfo->Callback(hWnd, rc.right, rc.bottom, (PWINDOWPOS)lParam);
        }
    }

    return 0;
}
