#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

MLE_API
VOID
NTAPI
UI_GetScreenPos(
    _Out_opt_ PPOINT Point,
    _Out_opt_ PSIZE Size);

EXTERN_C_END
