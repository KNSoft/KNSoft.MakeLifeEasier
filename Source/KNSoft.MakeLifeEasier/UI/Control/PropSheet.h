#pragma once

#include "../../MakeLifeEasier.h"

EXTERN_C_START

typedef struct _UI_PROPSHEET_PAGE
{
    HWND PageWindow;
    PCWSTR TabTitle;
} UI_PROPSHEET_PAGE, *PUI_PROPSHEET_PAGE;

MLE_API
LOGICAL
NTAPI
UI_InitPropSheetEx(
    _In_ HWND Dialog,
    _In_ INT TabControlId,
    _In_reads_(PageCount) UI_PROPSHEET_PAGE Pages[],
    _In_ UINT PageCount);

#define UI_InitPropSheet(Dialog, TabControlId, Pages) UI_InitPropSheetEx(Dialog, TabControlId, Pages, ARRAYSIZE(Pages))

MLE_API
INT_PTR
CALLBACK
UI_PropSheetWndProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    INT iTabControlId);

EXTERN_C_END
