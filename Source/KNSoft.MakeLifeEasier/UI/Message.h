#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

/// <summary>
/// Allow WM_DROPFILES and WM_COPYGLOBALDATA message, especially in an elevated application
/// </summary>
FORCEINLINE
W32ERROR
UI_AllowDrop(
    _In_ HWND Window)
{
    return (ChangeWindowMessageFilterEx(Window, WM_DROPFILES, MSGFLT_ADD, NULL) &&
            ChangeWindowMessageFilterEx(Window, WM_COPYGLOBALDATA, MSGFLT_ADD, NULL)) ?
        ERROR_SUCCESS : Err_GetLastError();
}

MLE_API
_Success_(return > 0)
ULONG
NTAPI
UI_GetWindowTextExW(
    _In_ HWND Window,
    _Out_writes_(TextCch) PWSTR Text,
    _In_ UINT TextCch);

MLE_API
_Success_(return > 0)
ULONG
NTAPI
UI_GetWindowTextExA(
    _In_ HWND Window,
    _Out_writes_(TextCch) PSTR Text,
    _In_ UINT TextCch);

#define UI_GetWindowTextW(Window, Text) UI_GetWindowTextExW(Window, Text, ARRAYSIZE(Text))
#define UI_GetWindowTextA(Window, Text) UI_GetWindowTextExA(Window, Text, ARRAYSIZE(Text))

/// <seealso cref="SendMessageTimeoutW"/>
FORCEINLINE
W32ERROR
UI_SendMessageTimeout(
    _In_ HWND Window,
    _In_ UINT Msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ UINT fuFlags,
    _In_ UINT uTimeout,
    _Out_opt_ PDWORD_PTR lpdwResult)
{
    LRESULT lr;
    W32ERROR Ret;

    Err_SetLastError(ERROR_SUCCESS);
    lr = SendMessageTimeoutW(Window, Msg, wParam, lParam, fuFlags, uTimeout, lpdwResult);
    if (lr != 0)
    {
        Ret = ERROR_SUCCESS;
    } else
    {
        Ret = Err_GetLastError();
        if (Ret == ERROR_SUCCESS)
        {
            Ret = ERROR_GEN_FAILURE;
        }
    }
    return Ret;
}

/// <seealso cref="SendMessage"/>
#define UI_SendMsgW(Window, Msg, wParam, lParam) SendMessageW(Window, Msg, (WPARAM)(wParam), (LPARAM)(lParam))
#define UI_SendMsgA(Window, Msg, wParam, lParam) SendMessageA(Window, Msg, (WPARAM)(wParam), (LPARAM)(lParam))
#define UI_LocalSendMsgW(Window, Msg, wParam, lParam) DefWindowProcW(Window, Msg, (WPARAM)(wParam), (LPARAM)(lParam))
#define UI_LocalSendMsgA(Window, Msg, wParam, lParam) DefWindowProcA(Window, Msg, (WPARAM)(wParam), (LPARAM)(lParam))

/// <seealso cref="SendDlgItemMessage"/>
#define UI_SendDlgItemMsgW(Dialog, ItemID, Msg, wParam, lParam) UI_SendMsgW(GetDlgItem(Dialog, ItemID), Msg, wParam, lParam)
#define UI_SendDlgItemMsgA(Dialog, ItemID, Msg, wParam, lParam) UI_SendMsgA(GetDlgItem(Dialog, ItemID), Msg, wParam, lParam)
#define UI_LocalSendDlgItemMsgW(Dialog, ItemID, Msg, wParam, lParam) UI_LocalSendMsgW(GetDlgItem(Dialog, ItemID), Msg, wParam, lParam)
#define UI_LocalSendDlgItemMsgA(Dialog, ItemID, Msg, wParam, lParam) UI_LocalSendMsgA(GetDlgItem(Dialog, ItemID), Msg, wParam, lParam)

/// <seealso cref="IsDlgButtonChecked"/>
#define UI_GetDlgButtonCheck(Dialog, ButtonID) SendMessageW(GetDlgItem(Dialog, ButtonID), BM_GETCHECK, 0, 0)
#define UI_LocalGetDlgButtonCheck(Dialog, ButtonID) DefWindowProcW(GetDlgItem(Dialog, ButtonID), BM_GETCHECK, 0, 0)

/// <seealso cref="CheckDlgButton"/>
#define UI_SetDlgButtonCheck(Dialog, ButtonID, CheckState) SendMessageW(GetDlgItem(Dialog, ButtonID), BM_SETCHECK, (WPARAM)(CheckState), 0)
#define UI_LocalSetDlgButtonCheck(Dialog, ButtonID, CheckState) DefWindowProcW(GetDlgItem(Dialog, ButtonID), BM_SETCHECK, (WPARAM)(CheckState), 0)

/// <seealso cref="SetWindowText"/>
#define UI_SetWindowTextW(Window, Text) SetWindowTextW(Window, Text)
#define UI_SetWindowTextA(Window, Text) SetWindowTextA(Window, Text)
#define UI_LocalSetWindowTextW(Window, Text) DefWindowProcW(Window, WM_SETTEXT, 0, (LPARAM)(Text))
#define UI_LocalSetWindowTextA(Window, Text) DefWindowProcA(Window, WM_SETTEXT, 0, (LPARAM)(Text))

/// <seealso cref="SetDlgItemText"/>
#define UI_SetDlgItemTextW(Dialog, ItemID, Text) UI_SetWindowTextW(GetDlgItem(Dialog, ItemID), Text)
#define UI_SetDlgItemTextA(Dialog, ItemID, Text) UI_SetWindowTextA(GetDlgItem(Dialog, ItemID), Text)
#define UI_LocalSetDlgItemTextW(Dialog, ItemID, Text) UI_LocalSetWindowTextW(GetDlgItem(Dialog, ItemID), Text)
#define UI_LocalSetDlgItemTextA(Dialog, ItemID, Text) UI_LocalSetWindowTextA(GetDlgItem(Dialog, ItemID), Text)

FORCEINLINE
VOID
UI_SetWindowFont(
    _In_ HWND Window,
    _In_opt_ HFONT Font,
    _In_ LOGICAL Redraw)
{
    SendMessageW(Window, WM_SETFONT, (WPARAM)Font, Redraw);
}

FORCEINLINE
HFONT
UI_GetWindowFont(
    _In_ HWND Window)
{
    return (HFONT)SendMessageW(Window, WM_GETFONT, 0, 0);
}

/// <summary>
/// Set window text without notify, useful to avoid endless loop caused by notification triggled when handling itself
/// </summary>

MLE_API
W32ERROR
NTAPI
UI_SetNoNotifyFlag(
    _In_ HWND Window,
    _In_ BOOL EnableState);

MLE_API
LOGICAL
NTAPI
UI_GetNoNotifyFlag(
    _In_ HWND Window);

MLE_API
LRESULT
NTAPI
UI_SetWndTextNoNotifyW(
    _In_ HWND Window,
    _In_opt_ PCWSTR Text);

MLE_API
LRESULT
NTAPI
UI_SetWndTextNoNotifyA(
    _In_ HWND Window,
    _In_opt_ PCSTR Text);

/* Message Loop */

FORCEINLINE
W32ERROR
UI_MessageLoop(
    _In_opt_ HWND Window,
    _In_ LOGICAL Dialog,
    _In_opt_ HACCEL Accelerator,
    _Out_opt_ PINT ExitCode)
{
    BOOL Ret;
    MSG Msg;

    while (TRUE)
    {
        Ret = GetMessageW(&Msg, Window, 0, 0);
        if (Ret == 0)
        {
            if (ExitCode != NULL)
            {
                *ExitCode = (INT)Msg.wParam;
            }
            return ERROR_SUCCESS;
        } else if (Ret == -1)
        {
            return Err_GetLastError();
        } else if ((Accelerator == NULL || !TranslateAcceleratorW(Msg.hwnd, Accelerator, &Msg)) &&
                   (!Dialog || !IsDialogMessageW(Msg.hwnd, &Msg)))
        {
            TranslateMessage(&Msg);
            DispatchMessageW(&Msg);
        }
    }
}

EXTERN_C_END
