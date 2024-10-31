#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

MLE_API
VOID
NTAPI
UI_SetDialogFont(
    _In_ HWND Dialog,
    _In_opt_ HFONT Font);

EXTERN_C_END
