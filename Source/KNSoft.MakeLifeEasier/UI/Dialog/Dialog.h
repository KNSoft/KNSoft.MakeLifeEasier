#pragma once

#include "../../MakeLifeEasier.h"

EXTERN_C_START

MLE_API
W32ERROR
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
    UIValueEditorEnum,
    UIValueEditorMax
} UI_VALUEEDITOR_TYPE, *PUI_VALUEEDITOR_TYPE;

MLE_API
W32ERROR
NTAPI
UI_ValueEditorDlg(
    _In_opt_ HWND Owner,
    _In_ UI_VALUEEDITOR_TYPE Type,
    _Inout_ PVOID Value,
    _In_ ULONG ValueSize,
    _In_reads_(ConstantCount) UI_VALUEEDITOR_CONSTANT Constants[],
    _In_ ULONG ConstantCount);

EXTERN_C_END
