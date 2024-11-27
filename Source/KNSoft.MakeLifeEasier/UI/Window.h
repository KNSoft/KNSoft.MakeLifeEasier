#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

FORCEINLINE
PVOID
UI_TruncateHandle(
    _In_ PVOID Handle)
{
#if _WIN64
    return (PVOID)(ULONG_PTR)(ULONG)(ULONG_PTR)Handle;
#else
    return Handle;
#endif
}

/// <seealso cref="PtInRect"/>
FORCEINLINE
LOGICAL
UI_PtInRect(
    _In_ PRECT Rect,
    _In_ PPOINT Point)
{
    return (Point->x >= Rect->left &&
            Point->x < Rect->right &&
            Point->y >= Rect->top &&
            Point->y < Rect->bottom);
}

/// <summary>
/// Gets position and size of virtual screen (multiple monitors support)
/// </summary>
FORCEINLINE
VOID
UI_GetScreenPos(
    _Out_opt_ PPOINT Point,
    _Out_opt_ PSIZE Size)
{
    if (Point)
    {
        Point->x = GetSystemMetrics(SM_XVIRTUALSCREEN);
        Point->y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    }
    if (Size)
    {
        Size->cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        Size->cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    }
}

/// <seealso cref="EnumChildWindows"/>
/// <remarks>Implemented by <c>GetWindow</c></remarks>
FORCEINLINE
VOID
UI_EnumChildWindows(
    _In_ HWND ParentWindow,
    _In_ WNDENUMPROC WindowEnumProc,
    _In_opt_ LPARAM Param)
{
    HWND hWndChild = GetWindow(ParentWindow, GW_CHILD);
    while (hWndChild != NULL && WindowEnumProc(hWndChild, Param))
    {
        hWndChild = GetWindow(hWndChild, GW_HWNDNEXT);
    }
}

/// <summary>
/// Invalidates and updates the whole window immediately
/// </summary>
/// <seealso cref="RedrawWindow"/>
FORCEINLINE
LOGICAL
UI_Redraw(
    _In_ HWND Window)
{
    return RedrawWindow(Window, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
}

FORCEINLINE
W32ERROR
UI_SetWindowRect(
    _In_ HWND Window,
    _In_ PRECT Rect,
    _In_ BOOL Redraw)
{
    return SetWindowPos(Window,
                        NULL,
                        Rect->left,
                        Rect->top,
                        Rect->right - Rect->left,
                        Rect->bottom - Rect->top,
                        SWP_NOZORDER | SWP_NOACTIVATE | (Redraw ? 0 : SWP_NOREDRAW)) ? ERROR_SUCCESS : NtGetLastError();
}

#pragma region Window Resize Subclass

typedef
_Function_class_(UI_WINDOW_RESIZE_FN)
VOID
CALLBACK
UI_WINDOW_RESIZE_FN(
    _In_ HWND Window,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_opt_ PWINDOWPOS MgmtWinPos);
typedef UI_WINDOW_RESIZE_FN *PUI_WINDOW_RESIZE_FN;

typedef struct _UI_WINDOW_RESIZE_INFO
{
    DWORD DPIX;
    DWORD DPIY;
    RECT Rect;
} UI_WINDOW_RESIZE_INFO, *PUI_WINDOW_RESIZE_INFO;

/* *prcMin should be initialized with zero */
MLE_API
INT_PTR
CALLBACK
UI_ResizeWndProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    _In_ LOGICAL bDialog,
    _In_opt_ __callback PUI_WINDOW_RESIZE_FN pfnResizeCallback,
    _Inout_ PUI_WINDOW_RESIZE_INFO pInfo);

#pragma endregion

#pragma region Theme

MLE_API
LOGICAL
NTAPI
UI_SetWindowThemeProperty(
    _In_ ATOM AtomTable,
    _In_ HWND Window,
    _In_opt_ PCWSTR Property);

FORCEINLINE
LOGICAL
UI_SetWindowTheme(
    _In_ HWND Window,
    _In_opt_ PCWSTR SubAppName,
    _In_opt_ PCWSTR SubIdList)
{
    LOGICAL Ret;

    Ret = UI_SetWindowThemeProperty(UXTHEME_ATOM_SUBAPPNAME, Window, SubAppName);
    Ret &= UI_SetWindowThemeProperty(UXTHEME_ATOM_SUBIDLIST, Window, SubIdList);
    SendMessageW(Window, WM_THEMECHANGED, 0, 0);

    return Ret;
}

/* Windows Explorer visual style can be applied to Tree-View, List-View, ... controls */
FORCEINLINE
LOGICAL
UI_SetWindowExplorerVisualStyle(
    _In_ HWND Window)
{
    return UI_SetWindowTheme(Window, L"Explorer", NULL);
}

FORCEINLINE
LOGICAL
UI_DisableWindowVisualStyle(
    _In_ HWND Window)
{
    return UI_SetWindowTheme(Window, L"", NULL);
}

#pragma endregion

#pragma region DWM

FORCEINLINE
HRESULT
UI_GetWindowRect(
    _In_ HWND Window,
    _Out_ PRECT Rect)
{
    /* DWM may extends frame for top-level windows */
    if (IsTopLevelWindow(Window) &&
        DwmGetWindowAttribute(Window, DWMWA_EXTENDED_FRAME_BOUNDS, Rect, sizeof(*Rect)) == S_OK)
    {
        return S_OK;
    }

    return GetWindowRect(Window, Rect) ? S_FALSE : HRESULT_FROM_WIN32(NtGetLastError());
}

FORCEINLINE
DWORD
UI_GetWindowCloackedState(
    _In_ HWND Window)
{
    DWORD State;

    return ((SharedUserData->NtMajorVersion > 6 ||
             (SharedUserData->NtMajorVersion == 6 && SharedUserData->NtMinorVersion > 1)) &&
            DwmGetWindowAttribute(Window, DWMWA_CLOAKED, &State, sizeof(State)) == S_OK) ? State : 0;
}

FORCEINLINE
HRESULT
UI_EnableWindowPeek(
    _In_ HWND Window,
    _In_ BOOL Enable)
{
    HRESULT hr;

    hr = DwmSetWindowAttribute(Window, DWMWA_DISALLOW_PEEK, &Enable, sizeof(Enable));
    if (FAILED(hr))
    {
        return hr;
    }

    /* In Win11, DWMWA_DISALLOW_PEEK seems not work */
    if (SharedUserData->NtMajorVersion > 10 ||
        (SharedUserData->NtMajorVersion == 10 && SharedUserData->NtBuildNumber >= 22000))
    {
        hr = DwmSetWindowAttribute(Window, DWMWA_FORCE_ICONIC_REPRESENTATION, &Enable, sizeof(Enable));
        return hr == S_OK ? S_FALSE : hr;
    }

    return S_OK;
}

#pragma endregion

/* Flash Window */

MLE_API
LOGICAL
NTAPI
UI_FlashWindow(
    _In_ HWND Window);

EXTERN_C_END
