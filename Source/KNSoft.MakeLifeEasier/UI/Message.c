#include "../MakeLifeEasier.inl"

W32ERROR
NTAPI
UI_AllowDrop(
    _In_ HWND Window)
{
    return (ChangeWindowMessageFilterEx(Window, WM_DROPFILES, MSGFLT_ADD, NULL) &&
            ChangeWindowMessageFilterEx(Window, WM_COPYGLOBALDATA, MSGFLT_ADD, NULL)) ?
        ERROR_SUCCESS : NtGetLastError();
}

_Success_(return > 0)
ULONG
NTAPI
UI_GetWindowTextExW(
    _In_ HWND Window,
    _Out_writes_z_(TextCch) PWSTR Text,
    _In_ UINT TextCch)
{
    ULONG cCh = (ULONG)GetWindowTextW(Window, Text, (INT)TextCch);
    if (cCh >= TextCch)
    {
        cCh = 0;
    }
    Text[cCh] = UNICODE_NULL;
    return cCh;
}

_Success_(return > 0)
ULONG
NTAPI
UI_GetWindowTextExA(
    _In_ HWND Window,
    _Out_writes_z_(TextCch) PSTR Text,
    _In_ UINT TextCch)
{
    ULONG cCh = (ULONG)GetWindowTextA(Window, Text, (INT)TextCch);
    if (cCh >= TextCch)
    {
        cCh = 0;
    }
    Text[cCh] = ANSI_NULL;
    return cCh;
}

#define UI_NONOTIFYPROP L"KNSoft.MakeLifeEasier.UI.NoNotify"

W32ERROR
NTAPI
UI_SetNoNotifyFlag(
    _In_ HWND Window,
    _In_ BOOL EnableState)
{
    if (EnableState)
    {
        return SetPropW(Window, UI_NONOTIFYPROP, (HANDLE)TRUE) ? ERROR_SUCCESS : NtGetLastError();
    } else
    {
        return RemovePropW(Window, UI_NONOTIFYPROP) != NULL ? ERROR_SUCCESS : NtGetLastError();
    }
}

LOGICAL
NTAPI
UI_GetNoNotifyFlag(
    _In_ HWND Window)
{
    return GetPropW(Window, UI_NONOTIFYPROP) == (HANDLE)TRUE;
}

LRESULT
NTAPI
UI_SetWndTextNoNotifyW(
    _In_ HWND Window,
    _In_opt_ PCWSTR Text)
{
    LRESULT Ret;

    UI_SetNoNotifyFlag(Window, TRUE);
    Ret = UI_SendMsgW(Window, WM_SETTEXT, 0, Text);
    UI_SetNoNotifyFlag(Window, FALSE);

    return Ret;
};

LRESULT
NTAPI
UI_SetWndTextNoNotifyA(
    _In_ HWND Window,
    _In_opt_ PCSTR Text)
{
    LRESULT Ret;

    UI_SetNoNotifyFlag(Window, TRUE);
    Ret = UI_SendMsgA(Window, WM_SETTEXT, 0, (LPARAM)Text);
    UI_SetNoNotifyFlag(Window, FALSE);

    return Ret;
};
