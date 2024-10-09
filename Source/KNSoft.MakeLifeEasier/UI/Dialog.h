#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

MLE_API
VOID
NTAPI
UI_SetDialogFont(
    _In_ HWND Dialog,
    _In_opt_ HFONT Font);

FORCEINLINE
INT
UI_MsgBox(
    _In_opt_ HWND Owner,
    _In_opt_ LPCWSTR Text,
    _In_opt_ LPCWSTR Title,
    _In_ UINT Type)
{
    return MessageBoxTimeoutW(Owner, Text, Title, Type, 0, -1);
}

EXTERN_C_END
