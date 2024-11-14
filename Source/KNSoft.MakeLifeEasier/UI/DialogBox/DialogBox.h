#pragma once

#include "../../MakeLifeEasier.h"

EXTERN_C_START

/* Common dialog box */

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

/* MakeLifeEasier dialog box */

MLE_API
HRESULT
NTAPI
UI_RectEditorDlg(
    _In_opt_ HWND Owner,
    _Inout_ PRECT Rect);

typedef struct _UI_VALUEEDITOR_CONSTANT
{
    UINT64  Value;
    PCWSTR  Name;
    PCWSTR  Description;
} UI_VALUEEDITOR_CONSTANT, *PUI_VALUEEDITOR_CONSTANT;

typedef enum _UI_VALUEEDITOR_TYPE
{
    UIValueEditorCombine,
    UIValueEditorEnum, // TODO: Unimplemented
    UIValueEditorMax
} UI_VALUEEDITOR_TYPE, *PUI_VALUEEDITOR_TYPE;

MLE_API
HRESULT
NTAPI
UI_ValueEditorDlg(
    _In_opt_ HWND Owner,
    _In_ UI_VALUEEDITOR_TYPE Type,
    _Inout_ PVOID Value,
    _In_ ULONG ValueSize,
    _In_reads_(ConstantCount) UI_VALUEEDITOR_CONSTANT Constants[],
    _In_ ULONG ConstantCount);

EXTERN_C_END
