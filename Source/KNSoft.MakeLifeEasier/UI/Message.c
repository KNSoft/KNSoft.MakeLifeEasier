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

W32ERROR
NTAPI
UI_DlgMessageLoop(
    _In_opt_ HWND Window,
    _In_ HWND Dialog,
    _In_opt_ HACCEL Accelerator,
    _Out_opt_ PINT_PTR ExitCode)
{
    BOOL bRet;
    MSG stMsg;

    while (TRUE)
    {
        bRet = GetMessageW(&stMsg, Window, 0, 0);
        if (bRet == 0)
        {
            if (ExitCode != NULL)
            {
                *ExitCode = (INT_PTR)stMsg.wParam;
            }
            return ERROR_SUCCESS;
        } else if (bRet == -1)
        {
            return NtGetLastError();
        } else if ((Accelerator == NULL || !TranslateAcceleratorW(Dialog, Accelerator, &stMsg)) &&
                   !IsDialogMessageW(Dialog, &stMsg))
        {
            TranslateMessage(&stMsg);
            DispatchMessageW(&stMsg);
        }
    }
}
