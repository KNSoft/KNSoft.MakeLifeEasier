#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

#pragma region Font

MLE_API
W32ERROR
NTAPI
UI_InitFontInfoEx(
    _Out_ PENUMLOGFONTEXDVW FontInfo,
    _In_ LONG Height,
    _In_ LONG Width,
    _In_ LONG Escapement,
    _In_ LONG Orientation,
    _In_ LONG Weight,
    _In_ BOOL Italic,
    _In_ BOOL Underline,
    _In_ BOOL StrikeOut,
    _In_ BYTE CharSet,
    _In_ BYTE OutPrecision,
    _In_ BYTE ClipPrecision,
    _In_ BYTE Quality,
    _In_ BYTE PitchAndFamily,
    _In_opt_ PCWSTR Name);

MLE_API
W32ERROR
NTAPI
UI_GetDefaultFont(
    _Out_ PENUMLOGFONTEXDVW FontInfo,
    _In_opt_ LONG NewHeight);

MLE_API
W32ERROR
NTAPI
UI_CreateDefaultFont(
    _Out_ HFONT* Font,
    _In_opt_ LONG NewHeight);

MLE_API
_Success_(return != FALSE)
LOGICAL
NTAPI
UI_GetFontInfo(
    _In_ HFONT Font,
    _Out_ PENUMLOGFONTEXDVW FontInfo);

#pragma endregion

EXTERN_C_END
