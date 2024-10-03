#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

#pragma region Font

_Success_(return != FALSE)
LOGICAL
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
    _In_opt_z_ PCWSTR Name);

_Success_(return != FALSE)
LOGICAL
NTAPI
UI_GetDefaultFont(
    _Out_ PENUMLOGFONTEXDVW FontInfo,
    _In_opt_ LONG NewHeight);

_Success_(return != NULL)
LOGICAL
NTAPI
UI_GetFontInfo(
    _In_ HFONT Font,
    _Out_ PENUMLOGFONTEXDVW FontInfo);

#pragma endregion

EXTERN_C_END
