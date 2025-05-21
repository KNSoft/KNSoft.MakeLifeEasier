#include "../MakeLifeEasier.inl"

_Success_(return > 0)
ULONG
NTAPI
UI_GetWindowTextExW(
    _In_ HWND Window,
    _Out_writes_(TextCch) PWSTR Text,
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
    _Out_writes_(TextCch) PSTR Text,
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

#define UI_NONOTIFY_PROP L"KNSoft.MakeLifeEasier.UI.NoNotify"

W32ERROR
NTAPI
UI_SetNoNotifyFlag(
    _In_ HWND Window,
    _In_ BOOL EnableState)
{
    if (EnableState)
    {
        return SetPropW(Window, UI_NONOTIFY_PROP, (HANDLE)TRUE) ? ERROR_SUCCESS : Err_GetLastError();
    } else
    {
        return RemovePropW(Window, UI_NONOTIFY_PROP) != NULL ? ERROR_SUCCESS : Err_GetLastError();
    }
}

LOGICAL
NTAPI
UI_GetNoNotifyFlag(
    _In_ HWND Window)
{
    return GetPropW(Window, UI_NONOTIFY_PROP) == (HANDLE)TRUE;
}

LRESULT
NTAPI
UI_SetWndTextNoNotifyW(
    _In_ HWND Window,
    _In_opt_ PCWSTR Text)
{
    LRESULT Ret;

    UI_SetNoNotifyFlag(Window, TRUE);
    Ret = UI_SetWindowTextW(Window, Text);
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
    Ret = UI_SetWindowTextA(Window, Text);
    UI_SetNoNotifyFlag(Window, FALSE);

    return Ret;
};
