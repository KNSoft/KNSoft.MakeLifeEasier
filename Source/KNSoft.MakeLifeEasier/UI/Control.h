#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

MLE_API
VOID
NTAPI
UI_SetDialogFont(
    _In_ HWND Dialog,
    _In_opt_ HFONT Font);

MLE_API
LOGICAL
NTAPI
UI_ListViewSort(
    _In_ HWND List,
    _In_ PFNLVCOMPARE CompareFn,
    _In_ INT ColumnIndex);

EXTERN_C_END
