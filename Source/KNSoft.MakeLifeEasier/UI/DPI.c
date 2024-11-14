#include "../MakeLifeEasier.inl"

#include <shellscalingapi.h>

#pragma region Scale

W32ERROR
NTAPI
UI_DPIScaleFont(
    _Inout_ HFONT* Font,
    _In_ UINT OldDPIY,
    _In_ UINT NewDPIY)
{
    ENUMLOGFONTEXDVW FontInfo;
    HFONT NewFont;

    if (OldDPIY == NewDPIY)
    {
        return ERROR_SUCCESS;
    }
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

typedef
BOOL
WINAPI
FN_AdjustWindowRectExForDpi(
    _Inout_ LPRECT lpRect,
    _In_ DWORD dwStyle,
    _In_ BOOL bMenu,
    _In_ DWORD dwExStyle,
    _In_ UINT dpi);

static FN_AdjustWindowRectExForDpi* g_pfnAdjustWindowRectExForDpi = NULL;
static CONST ANSI_STRING g_asAdjustWindowRectExForDpi = RTL_CONSTANT_STRING("AdjustWindowRectExForDpi");

W32ERROR
NTAPI
UI_DPIAdjustWindowRect(
    _In_ HWND Window,
    _Inout_ PRECT Rect,
    _In_ UINT DPI)
{
    DWORD Style, ExStyle;
    BOOL HasMenu;

    Style = (DWORD)GetWindowLongPtrW(Window, GWL_STYLE);
    ExStyle = (DWORD)GetWindowLongPtrW(Window, GWL_EXSTYLE);
    HasMenu = GetMenu(Window) != NULL;

    /* AdjustWindowRectExForDpi since Win10 1607 (Redstone, 10.0.14393) */
    if (SharedUserData->NtMajorVersion < 10 ||
        (SharedUserData->NtMajorVersion == 10 && SharedUserData->NtBuildNumber < 14393))
    {
        goto _Fallback;
    }
    if (g_pfnAdjustWindowRectExForDpi == NULL)
    {
        if (!NT_SUCCESS(Sys_LoadProcByName(SysLibUser32,
                                           &g_asAdjustWindowRectExForDpi,
                                           (PVOID*)&g_pfnAdjustWindowRectExForDpi)))
        {
            goto _Fallback;
        }
    }
    if (g_pfnAdjustWindowRectExForDpi(Rect, Style, HasMenu, ExStyle, DPI))
    {
        return ERROR_SUCCESS;
    }

_Fallback:
    return AdjustWindowRectEx(Rect, Style, HasMenu, ExStyle) ? ERROR_SUCCESS : NtGetLastError();
}

static
W32ERROR
NTAPI
UI_DPIScaleDialog(
    _In_ HWND Dialog,
    _In_ UINT OldDPIX,
    _In_ UINT NewDPIX,
    _In_ UINT OldDPIY,
    _In_ UINT NewDPIY,
    _In_opt_ PRECT SuggestRect,
    _When_(ScaleFont != FALSE, _Inout_opt_) _When_(ScaleFont == FALSE, _In_opt_) HFONT* Font,
    _In_ LOGICAL ScaleFont,
    _In_ LOGICAL Redraw)
{
    W32ERROR eRet;
    RECT rcDlg, rc, *prcDlg;
    LONG lDelta;
    POINT ptOrigin = { 0, 0 };
    HFONT hFont;
    HWND hWnd;

    /* Adjust dialog rect */
    if (SuggestRect == NULL)
    {
        if (!GetClientRect(Dialog, &rcDlg) || !ClientToScreen(Dialog, &ptOrigin))
        {
            return NtGetLastError();
        }
        rcDlg.left += ptOrigin.x;
        rcDlg.right += ptOrigin.x;
        rcDlg.top += ptOrigin.y;
        rcDlg.bottom += ptOrigin.y;

        lDelta = Math_RoundInt((rcDlg.right - rcDlg.left) * (((FLOAT)NewDPIX / OldDPIX) - 1) / 2);
        rcDlg.left -= lDelta;
        rcDlg.right += lDelta;
        lDelta = Math_RoundInt((rcDlg.bottom - rcDlg.top) * (((FLOAT)NewDPIY / OldDPIY) - 1) / 2);
        rcDlg.top -= lDelta;
        rcDlg.bottom += lDelta;

        UI_DPIAdjustWindowRect(Dialog, &rcDlg, NewDPIX);
        prcDlg = &rcDlg;
    } else
    {
        prcDlg = SuggestRect;
    }
    eRet = UI_SetWindowRect(Dialog, prcDlg, FALSE);
    if (eRet != ERROR_SUCCESS)
    {
        return eRet;
    }

    /* Adjust font */
    if (Font != NULL)
    {
        if (ScaleFont)
        {
            eRet = UI_DPIScaleFont(Font, OldDPIY, NewDPIY);
            if (eRet != ERROR_SUCCESS)
            {
                return eRet;
            }
        }
        hFont = *Font;
    } else
    {
        hFont = NULL;
    }

    /* Adjust child windows */
    ptOrigin.x = ptOrigin.y = 0;
    ClientToScreen(Dialog, &ptOrigin);
    hWnd = GetWindow(Dialog, GW_CHILD);
    while (hWnd != NULL)
    {
        if (hFont != NULL)
        {
            UI_SetWindowFont(hWnd, hFont, FALSE);
        }
        if (GetWindowRect(hWnd, &rc))
        {
            rc.left -= ptOrigin.x;
            rc.right -= ptOrigin.x;
            rc.top -= ptOrigin.y;
            rc.bottom -= ptOrigin.y;
            UI_DPIScaleRect(&rc, OldDPIX, NewDPIX, OldDPIY, NewDPIY);
            UI_SetWindowRect(hWnd, &rc, FALSE);
        }
        hWnd = GetWindow(hWnd, GW_HWNDNEXT);
    }

    /* Send size changed notify to dialog so that it could re-layout child windows */
    GetClientRect(Dialog, &rcDlg);
    SendMessageW(Dialog, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rcDlg.right, rcDlg.bottom));
    if (Redraw)
    {
        UI_Redraw(Dialog);
    }
    return ERROR_SUCCESS;
}

#pragma endregion

#pragma region Dialog Auto Scaling

#define UI_DIALOG_DPISCALE_PROP L"KNSoft.MakeLifeEasier.UI.DialogDPIScale"

INT_PTR
CALLBACK
UI_DPIScaleDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG)
    {
        W32ERROR Ret;
        PUI_DIALOG_DPI_INFO DPIInfo;

        if (!Mem_AllocPtr(DPIInfo))
        {
            return 0;
        }

        /* Default font is already DPI scaled */
        if (UI_CreateDefaultFont(&DPIInfo->Font, 0) != ERROR_SUCCESS)
        {
            DPIInfo->Font = NULL;
        }

        UI_GetWindowDPI(hDlg, &DPIInfo->DPIX, &DPIInfo->DPIY);
        Ret = UI_DPIScaleDialog(hDlg,
                                USER_DEFAULT_SCREEN_DPI,
                                DPIInfo->DPIX,
                                USER_DEFAULT_SCREEN_DPI,
                                DPIInfo->DPIY,
                                NULL,
                                &DPIInfo->Font,
                                FALSE,
                                FALSE);
        if (Ret == ERROR_SUCCESS)
        {
            if (SetPropW(hDlg, UI_DIALOG_DPISCALE_PROP, (HANDLE)DPIInfo))
            {
                /* Success */
                return 0;
            }
            Ret = NtGetLastError();
        }

        if (DPIInfo->Font != NULL)
        {
            DeleteObject(DPIInfo->Font);
        }
        Mem_Free(DPIInfo);
    } else if (uMsg == WM_DPICHANGED)
    {
        UINT DPIX, DPIY;
        PRECT prc;
        PUI_DIALOG_DPI_INFO DPIInfo = GetPropW(hDlg, UI_DIALOG_DPISCALE_PROP);

        if (DPIInfo)
        {
            /* FIXME: Seems given lParam not right in PMv2 mode? */
            prc = UI_CompareDPIAwareContext(UI_GetWindowDPIAwareContext(hDlg),
                                            DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) ? NULL : (PRECT)lParam;

            DPIX = LOWORD(wParam);
            DPIY = HIWORD(wParam);
            if (UI_DPIScaleDialog(hDlg,
                                  DPIInfo->DPIX,
                                  DPIX,
                                  DPIInfo->DPIY,
                                  DPIY,
                                  prc,
                                  &DPIInfo->Font,
                                  TRUE,
                                  TRUE) == ERROR_SUCCESS)
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

#pragma endregion

#pragma region DPI Awareness Context

typedef
DPI_AWARENESS_CONTEXT
WINAPI
FN_SetThreadDpiAwarenessContext(
    _In_ DPI_AWARENESS_CONTEXT dpiContext);

static FN_SetThreadDpiAwarenessContext* g_pfnSetThreadDpiAwarenessContext = NULL;
static CONST ANSI_STRING g_asSetThreadDpiAwarenessContext = RTL_CONSTANT_STRING("SetThreadDpiAwarenessContext");

typedef
DPI_AWARENESS_CONTEXT
WINAPI
FN_GetThreadDpiAwarenessContext(VOID);

static FN_GetThreadDpiAwarenessContext* g_pfnGetThreadDpiAwarenessContext = NULL;
static CONST ANSI_STRING g_asGetThreadDpiAwarenessContext = RTL_CONSTANT_STRING("GetThreadDpiAwarenessContext");

typedef
BOOL
WINAPI
FN_AreDpiAwarenessContextsEqual(
    _In_ DPI_AWARENESS_CONTEXT dpiContextA,
    _In_ DPI_AWARENESS_CONTEXT dpiContextB);

static FN_AreDpiAwarenessContextsEqual* g_pfnAreDpiAwarenessContextsEqual = NULL;
static CONST ANSI_STRING g_asAreDpiAwarenessContextsEqual = RTL_CONSTANT_STRING("AreDpiAwarenessContextsEqual");

typedef
DPI_AWARENESS_CONTEXT
WINAPI
FN_GetWindowDpiAwarenessContext(
    _In_ HWND hwnd);

static FN_GetWindowDpiAwarenessContext* g_pfnGetWindowDpiAwarenessContext = NULL;
static CONST ANSI_STRING g_asGetWindowDpiAwarenessContext = RTL_CONSTANT_STRING("GetWindowDpiAwarenessContext");

typedef
HRESULT
STDAPICALLTYPE
FN_GetProcessDpiAwareness(
    _In_opt_ HANDLE hprocess,
    _Out_ PROCESS_DPI_AWARENESS* value);

static FN_GetProcessDpiAwareness* g_pfnGetProcessDpiAwareness = NULL;
static CONST ANSI_STRING g_asGetProcessDpiAwareness = RTL_CONSTANT_STRING("GetProcessDpiAwareness");

_Success_(return != NULL)
DPI_AWARENESS_CONTEXT
NTAPI
UI_EnableDPIAwareContext(VOID)
{
    /* SetThreadDpiAwarenessContext since Win10 1607 (Redstone, 10.0.14393) */
    if (SharedUserData->NtMajorVersion < 10 ||
        (SharedUserData->NtMajorVersion == 10 && SharedUserData->NtBuildNumber < 14393))
    {
        return NULL;
    }
    if (g_pfnSetThreadDpiAwarenessContext == NULL)
    {
        if (!NT_SUCCESS(Sys_LoadProcByName(SysLibUser32,
                                           &g_asSetThreadDpiAwarenessContext,
                                           (PVOID*)&g_pfnSetThreadDpiAwarenessContext)))
        {
            return NULL;
        }
    }

    /* PMv2 since Win10 1703 (Redstone 2, 10.0.15063) */
    return g_pfnSetThreadDpiAwarenessContext(
        (SharedUserData->NtMajorVersion > 10 || SharedUserData->NtBuildNumber >= 15063) ?
        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 : DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
}

_Success_(return != NULL)
DPI_AWARENESS_CONTEXT
NTAPI
UI_RestoreDPIAwareContext(
    _In_opt_ DPI_AWARENESS_CONTEXT Context)
{
    if (Context == NULL)
    {
        return NULL;
    }
    return g_pfnSetThreadDpiAwarenessContext(Context);
}

_Success_(return != NULL)
DPI_AWARENESS_CONTEXT
NTAPI
UI_GetDPIAwareContext(VOID)
{
    /* GetThreadDpiAwarenessContext since Win10 1607 (Redstone, 10.0.14393) */
    if (SharedUserData->NtMajorVersion < 10 ||
        (SharedUserData->NtMajorVersion == 10 && SharedUserData->NtBuildNumber < 14393))
    {
        return NULL;
    }
    if (g_pfnGetThreadDpiAwarenessContext == NULL)
    {
        if (!NT_SUCCESS(Sys_LoadProcByName(SysLibUser32,
                                           &g_asGetThreadDpiAwarenessContext,
                                           (PVOID*)&g_pfnGetThreadDpiAwarenessContext)))
        {
            return NULL;
        }
    }
    return g_pfnGetThreadDpiAwarenessContext();
}

DPI_AWARENESS_CONTEXT
NTAPI
UI_GetWindowDPIAwareContext(
    _In_ HWND Window)
{
    PROCESS_DPI_AWARENESS Awareness;

    /* GetWindowDpiAwarenessContext since Win10 1607 (Redstone, 10.0.14393) */
    if (SharedUserData->NtMajorVersion < 10 ||
        (SharedUserData->NtMajorVersion == 10 && SharedUserData->NtBuildNumber < 14393))
    {
        goto _Fallback_Win_8_1;
    }
    if (g_pfnGetWindowDpiAwarenessContext == NULL)
    {
        if (!NT_SUCCESS(Sys_LoadProcByName(SysLibUser32,
                                           &g_asGetWindowDpiAwarenessContext,
                                           (PVOID*)&g_pfnGetWindowDpiAwarenessContext)))
        {
            goto _Fallback_Win_8_1;
        }
    }
    return g_pfnGetWindowDpiAwarenessContext(Window);

_Fallback_Win_8_1:
    if (SharedUserData->NtMajorVersion < 8 ||
        (SharedUserData->NtMajorVersion == 8 && SharedUserData->NtMinorVersion < 1))
    {
        goto _Fallback_Win_6_0;
    }
    if (g_pfnGetProcessDpiAwareness == NULL)
    {
        if (!NT_SUCCESS(Sys_LoadProcByName(SysLibShcore,
                                           &g_asGetProcessDpiAwareness,
                                           (PVOID*)&g_pfnGetProcessDpiAwareness)))
        {
            goto _Fallback_Win_6_0;
        }
    }
    if (g_pfnGetProcessDpiAwareness(NULL, &Awareness) == S_OK)
    {
        if (Awareness == PROCESS_DPI_UNAWARE)
        {
            return DPI_AWARENESS_CONTEXT_UNAWARE;
        } if (Awareness == PROCESS_SYSTEM_DPI_AWARE)
        {
            return DPI_AWARENESS_CONTEXT_SYSTEM_AWARE;
        } else if (Awareness == PROCESS_PER_MONITOR_DPI_AWARE)
        {
            return DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE;
        }
    }
    return NULL;

_Fallback_Win_6_0:
    return IsProcessDPIAware() ? DPI_AWARENESS_CONTEXT_SYSTEM_AWARE : DPI_AWARENESS_CONTEXT_UNAWARE;
}

LOGICAL
NTAPI
UI_CompareDPIAwareContext(
    _In_ DPI_AWARENESS_CONTEXT Context1,
    _In_ DPI_AWARENESS_CONTEXT Context2)
{
    /* AreDpiAwarenessContextsEqual since Win10 1607 (Redstone, 10.0.14393) */
    if (SharedUserData->NtMajorVersion < 10 ||
        (SharedUserData->NtMajorVersion == 10 && SharedUserData->NtBuildNumber < 14393))
    {
        return FALSE;
    }
    if (g_pfnAreDpiAwarenessContextsEqual == NULL)
    {
        if (!NT_SUCCESS(Sys_LoadProcByName(SysLibUser32,
                                           &g_asAreDpiAwarenessContextsEqual,
                                           (PVOID*)&g_pfnAreDpiAwarenessContextsEqual)))
        {
            return FALSE;
        }
    }
    return g_pfnAreDpiAwarenessContextsEqual(Context1, Context2);
}

#pragma endregion
