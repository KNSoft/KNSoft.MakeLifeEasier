#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

/// <summary>
/// Gets position and size of virtual screen (multiple monitors support)
/// </summary>
MLE_API
VOID
NTAPI
UI_GetScreenPos(
    _Out_opt_ PPOINT Point,
    _Out_opt_ PSIZE Size);

EXTERN_C_END
