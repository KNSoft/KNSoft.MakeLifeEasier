#pragma once

#include "../../MakeLifeEasier.h"

#include "../Message.h"

EXTERN_C_START

FORCEINLINE
VOID
UI_SetDialogFont(
    _In_ HWND Dialog,
    _In_opt_ HFONT Font)
{
    HWND hWnd = GetWindow(Dialog, GW_CHILD);
    while (hWnd)
    {
        UI_SetWindowFont(hWnd, Font, FALSE);
        hWnd = GetWindow(hWnd, GW_HWNDNEXT);
    }
    UI_Redraw(Dialog);
}

/// <seealso cref="EnableWindow"/>
FORCEINLINE
LOGICAL
UI_EnableDlgItem(
    _In_ HWND Dialog,
    _In_ INT ItemId,
    _In_ LOGICAL EnableState)
{
    return EnableWindow(GetDlgItem(Dialog, ItemId), EnableState);
}

EXTERN_C_END
