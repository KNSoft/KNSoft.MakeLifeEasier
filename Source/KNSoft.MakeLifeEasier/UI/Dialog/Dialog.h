#pragma once

#include "../../MakeLifeEasier.h"

EXTERN_C_START

MLE_API
W32ERROR
NTAPI
UI_RectEditorDlg(
    _In_opt_ HWND Owner,
    _Inout_ PRECT Rect);

EXTERN_C_END
