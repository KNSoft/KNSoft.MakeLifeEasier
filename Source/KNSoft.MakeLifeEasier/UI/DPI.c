#include "../MakeLifeEasier.inl"

VOID
NTAPI
UI_GetWindowDPI(
    _In_ HWND Window,
    _Out_ PUINT DPIX,
    _Out_ PUINT DPIY)
{
    HDC hDC;

    hDC = GetDC(Window);
    *DPIX = GetDeviceCaps(hDC, LOGPIXELSX);
    *DPIY = GetDeviceCaps(hDC, LOGPIXELSY);
    ReleaseDC(Window, hDC);
}

VOID
NTAPI
UI_DPIScaleRect(
    _Inout_ PRECT Rect,
    _In_ UINT OldDPIX,
    _In_ UINT NewDPIX,
    _In_ UINT OldDPIY,
    _In_ UINT NewDPIY)
{
    UI_DPIScaleInt(&Rect->left, OldDPIX, NewDPIX);
    UI_DPIScaleInt(&Rect->right, OldDPIX, NewDPIX);
    UI_DPIScaleInt(&Rect->top, OldDPIY, NewDPIY);
    UI_DPIScaleInt(&Rect->bottom, OldDPIY, NewDPIY);
}

W32ERROR
NTAPI
UI_DPIScaleFont(
    _Inout_ HFONT* Font,
    _In_ UINT OldDPIY,
    _In_ UINT NewDPIY)
{
    ENUMLOGFONTEXDVW FontInfo;
    HFONT NewFont;

    if (!UI_GetFontInfo(*Font, &FontInfo))
    {
        return ERROR_INVALID_PARAMETER;
    }

    FontInfo.elfEnumLogfontEx.elfLogFont.lfWidth = 0;
    UI_DPIScaleInt(&FontInfo.elfEnumLogfontEx.elfLogFont.lfHeight, OldDPIY, NewDPIY);
    NewFont = CreateFontIndirectExW(&FontInfo);
    if (NewFont == NULL)
    {
        return NtGetLastError();
    }

    DeleteObject(*Font);
    *Font = NewFont;
    return ERROR_SUCCESS;
}

W32ERROR
NTAPI
UI_DPIScaleDialogRect(
    _In_ HWND Dialog,
    _In_ UINT OldDPIX,
    _In_ UINT NewDPIX,
    _In_ UINT OldDPIY,
    _In_ UINT NewDPIY,
    _In_ LOGICAL Redraw)
{
    LONG lDelta;
    RECT rc;
    POINT pt = { 0 };

    if (!GetClientRect(Dialog, &rc) || !ClientToScreen(Dialog, &pt))
    {
        return NtGetLastError();
    }

    rc.left += pt.x;
    rc.right += pt.x;
    rc.top += pt.y;
    rc.bottom += pt.y;
    if (!AdjustWindowRectEx(&rc,
                            (DWORD)GetWindowLongPtrW(Dialog, GWL_STYLE),
                            GetMenu(Dialog) != NULL,
                            (DWORD)GetWindowLongPtrW(Dialog, GWL_EXSTYLE)))
    {
        return NtGetLastError();
    }

    lDelta = Math_RoundInt((rc.right - rc.left) * (((FLOAT)NewDPIX / OldDPIX) - 1) / 2);
    if (lDelta <= rc.left)
    {
        rc.left -= lDelta;
        rc.right += lDelta;
    } else
    {
        rc.right += 2 * lDelta - rc.left;
        rc.left = 0;
    }
    lDelta = Math_RoundInt((rc.bottom - rc.top) * (((FLOAT)NewDPIY / OldDPIY) - 1) / 2);
    if (lDelta <= rc.top)
    {
        rc.top -= lDelta;
        rc.bottom += lDelta;
    } else
    {
        rc.bottom += 2 * lDelta - rc.top;
        rc.top = 0;
    }

    return UI_SetWindowRect(Dialog, &rc, Redraw);
}

W32ERROR
NTAPI
UI_DPIScaleChildRect(
    _In_ HWND Window,
    _In_ PPOINT ParentScreenOrigin,
    _In_ UINT OldDPIX,
    _In_ UINT NewDPIX,
    _In_ UINT OldDPIY,
    _In_ UINT NewDPIY,
    _In_ LOGICAL Redraw)
{
    RECT rc;

    if (!GetWindowRect(Window, &rc))
    {
        return NtGetLastError();
    }
    rc.left -= ParentScreenOrigin->x;
    rc.right -= ParentScreenOrigin->x;
    rc.top -= ParentScreenOrigin->y;
    rc.bottom -= ParentScreenOrigin->y;
    UI_DPIScaleRect(&rc, OldDPIX, NewDPIX, OldDPIY, NewDPIY);

    return UI_SetWindowRect(Window, &rc, Redraw);
}

typedef struct _UI_DPISCALEDIALOG_UPATE_CHILD_INFO
{
    PUI_DIALOG_DPI_INFO DPIInfo;
    POINT               DialogScreenOrigin;
    DWORD               NewDPIX;
    DWORD               NewDPIY;
} UI_DPISCALEDIALOG_UPATE_CHILD_INFO, *PUI_DPISCALEDIALOG_UPATE_CHILD_INFO;

static
BOOL
CALLBACK
UI_DPIScaleDialog_EnumChild_Proc(
    HWND hCtl,
    LPARAM lParam)
{
    PUI_DPISCALEDIALOG_UPATE_CHILD_INFO pInfo = (PUI_DPISCALEDIALOG_UPATE_CHILD_INFO)lParam;

    if (pInfo->DPIInfo->Font != NULL)
    {
        UI_SetWindowFont(hCtl, pInfo->DPIInfo->Font, FALSE);
    }
    UI_DPIScaleChildRect(hCtl,
                         &pInfo->DialogScreenOrigin,
                         pInfo->DPIInfo->DPIX,
                         pInfo->NewDPIX,
                         pInfo->DPIInfo->DPIY,
                         pInfo->NewDPIY,
                         FALSE);

    return TRUE;
}

W32ERROR
NTAPI
UI_DPIScaleDialog(
    _In_ HWND Dialog,
    _In_ UINT OldDPIX,
    _In_ UINT NewDPIX,
    _In_ UINT OldDPIY,
    _In_ UINT NewDPIY,
    _In_opt_ PRECT SuggestedPos,
    _Inout_opt_ HFONT* Font)
{
    W32ERROR Error;
    UI_DIALOG_DPI_INFO DPIInfo;
    UI_DPISCALEDIALOG_UPATE_CHILD_INFO DPIUpdateInfo;

    /* Adjust dialog rect */
    if (SuggestedPos != NULL)
    {
        Error = UI_SetWindowRect(Dialog, SuggestedPos, FALSE);
    } else
    {
        Error = UI_DPIScaleDialogRect(Dialog, OldDPIX, NewDPIX, OldDPIY, NewDPIY, FALSE);
    }
    if (Error != ERROR_SUCCESS)
    {
        return Error;
    }
    DPIUpdateInfo.DialogScreenOrigin.x = DPIUpdateInfo.DialogScreenOrigin.y = 0;
    if (!ClientToScreen(Dialog, &DPIUpdateInfo.DialogScreenOrigin))
    {
        return NtGetLastError();
    }

    /* Adjust font */
    if (Font != NULL)
    {
        Error = UI_DPIScaleFont(Font, OldDPIY, NewDPIY);
        if (Error != ERROR_SUCCESS)
        {
            return Error;
        }
        DPIInfo.Font = *Font;
    } else
    {
        DPIInfo.Font = NULL;
    }

    /* Adjust child windows */
    DPIInfo.DPIX = OldDPIX;
    DPIInfo.DPIY = OldDPIY;
    DPIUpdateInfo.DPIInfo = &DPIInfo;
    DPIUpdateInfo.NewDPIX = NewDPIX;
    DPIUpdateInfo.NewDPIY = NewDPIY;
    UI_EnumChildWindows(Dialog, UI_DPIScaleDialog_EnumChild_Proc, (LPARAM)&DPIUpdateInfo);

    UI_Redraw(Dialog);
    return ERROR_SUCCESS;
}

#define UI_DIALOG_DPISCALE_PROP L"KNSoft.MakeLifeEasier.UI.DialogDPIScale"

static
W32ERROR
NTAPI
UI_InitializeDialogDPIScale(
    _In_ HWND Dialog)
{
    W32ERROR Ret;
    PUI_DIALOG_DPI_INFO DPIInfo;
    UI_DPISCALEDIALOG_UPATE_CHILD_INFO DPIUpdateInfo;

    if (!NT_SUCCESS(Mem_AllocPtr(DPIInfo)))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    /* Scale dialog itself */
    UI_GetWindowDPI(Dialog, &DPIUpdateInfo.NewDPIX, &DPIUpdateInfo.NewDPIY);
    Ret = UI_DPIScaleDialogRect(Dialog,
                                USER_DEFAULT_SCREEN_DPI,
                                DPIUpdateInfo.NewDPIX,
                                USER_DEFAULT_SCREEN_DPI,
                                DPIUpdateInfo.NewDPIY, FALSE);
    if (Ret != ERROR_SUCCESS)
    {
        return Ret;
    }
    DPIUpdateInfo.DialogScreenOrigin.x = DPIUpdateInfo.DialogScreenOrigin.y = 0;
    if (!ClientToScreen(Dialog, &DPIUpdateInfo.DialogScreenOrigin))
    {
        return NtGetLastError();
    }

    /* Default font is already DPI scaled */
    if (UI_CreateDefaultFont(&DPIInfo->Font, 0) != ERROR_SUCCESS)
    {
        DPIInfo->Font = NULL;
    }

    /* Adjust child windows */
    DPIInfo->DPIX = DPIInfo->DPIY = USER_DEFAULT_SCREEN_DPI;
    DPIUpdateInfo.DPIInfo = DPIInfo;
    UI_EnumChildWindows(Dialog, UI_DPIScaleDialog_EnumChild_Proc, (LPARAM)&DPIUpdateInfo);

    DPIInfo->DPIX = DPIUpdateInfo.NewDPIX;
    DPIInfo->DPIY = DPIUpdateInfo.NewDPIY;
    return SetPropW(Dialog, UI_DIALOG_DPISCALE_PROP, (HANDLE)DPIInfo) ? ERROR_SUCCESS : NtGetLastError();
}

INT_PTR
CALLBACK
UI_DPIScaleDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG)
    {
        UI_InitializeDialogDPIScale(hDlg);
    } else if (uMsg == WM_DPICHANGED)
    {
        UINT DPIX = LOWORD(wParam), DPIY = HIWORD(wParam);
        PUI_DIALOG_DPI_INFO DPIInfo = GetPropW(hDlg, UI_DIALOG_DPISCALE_PROP);

        if (DPIInfo)
        {
            if (UI_DPIScaleDialog(hDlg,
                                  DPIInfo->DPIX,
                                  DPIX,
                                  DPIInfo->DPIY,
                                  DPIY,
                                  (PRECT)lParam,
                                  &DPIInfo->Font) == ERROR_SUCCESS)
            {
                DPIInfo->DPIX = DPIX;
                DPIInfo->DPIY = DPIY;
            }
        }
        SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, 0);
    } else if (uMsg == WM_DESTROY)
    {
        PUI_DIALOG_DPI_INFO DPIInfo;

        DPIInfo = GetPropW(hDlg, UI_DIALOG_DPISCALE_PROP);
        if (DPIInfo != NULL)
        {
            if (DPIInfo->Font != NULL)
            {
                DeleteObject(DPIInfo->Font);
            }
            Mem_Free(DPIInfo);
            RemovePropW(hDlg, UI_DIALOG_DPISCALE_PROP);
        }
    }
    return 0;
}

_Success_(return != NULL)
_Ret_maybenull_
PUI_DIALOG_DPI_INFO
NTAPI
UI_GetDialogDPIScaleInfo(
    _In_ HWND Dialog)
{
    return GetPropW(Dialog, UI_DIALOG_DPISCALE_PROP);
}

_Success_(return != NULL)
PVOID
NTAPI
UI_EnableDPIAwareContext(VOID)
{
    /* TODO */

    DPI_AWARENESS_CONTEXT Context;

    /* PMv2 since Win10 1703 (Redstone 2, 10.0.15063) */
    if (SharedUserData->NtMajorVersion < 10 || SharedUserData->NtBuildNumber < 15063)
    {
        return NULL;
    }

    Context = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    return Context;
}

LOGICAL
NTAPI
UI_RestoreDPIAwareContext(
    _In_ PVOID Cookie)
{
    /* TODO */
    return SetThreadDpiAwarenessContext(Cookie) != NULL;
}
