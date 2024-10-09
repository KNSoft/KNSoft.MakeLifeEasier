#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

/// <summary>
/// Gets position and size of virtual screen (multiple monitors support)
/// </summary>
MLE_API
VOID
NTAPI
UI_GetScreenPos(
    _Out_opt_ PPOINT Point,
    _Out_opt_ PSIZE Size);

/// <seealso cref="EnumChildWindows"/>
/// <remarks>Implemented by <c>GetWindow</c></remarks>
MLE_API
VOID
NTAPI
UI_EnumChildWindows(
    _In_ HWND ParentWindow,
    _In_ WNDENUMPROC WindowEnumProc,
    _In_opt_ LPARAM Param);

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

MLE_API
W32ERROR
NTAPI
UI_SetWindowRect(
    _In_ HWND Window,
    _In_ PRECT Rect,
    _In_ BOOL Redraw);

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

MLE_API
W32ERROR
NTAPI
UI_InitializeWindowResize(
    _In_ HWND Window,
    _In_opt_ LONG MinWidth,
    _In_opt_ LONG MinHeight,
    _In_opt_ PUI_WINDOW_RESIZE_FN Callback);

MLE_API
VOID
NTAPI
UI_UninitializeWindowResize(
    _In_ HWND Window);

MLE_API
INT_PTR
CALLBACK
UI_ResizeWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#pragma endregion

EXTERN_C_END
