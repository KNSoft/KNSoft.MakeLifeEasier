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

#pragma region Paint

FORCEINLINE
LOGICAL
GDI_FillSolidRect(
    _In_ HDC DC,
    _In_ PRECT Rect,
    _In_ COLORREF Color)
{
    COLORREF OldColor = SetBkColor(DC, Color);
    LOGICAL Ret = ExtTextOutW(DC, 0, 0, ETO_OPAQUE, Rect, NULL, 0, NULL);
    SetBkColor(DC, OldColor);
    return Ret;
}

FORCEINLINE
LOGICAL
GDI_FrameRect(
    _In_ HDC DC,
    _In_ PRECT Rect,
    _In_ INT Width,
    _In_ DWORD ROP)
{
    return Width >= 0
        ?
        PatBlt(DC, Rect->left - Width, Rect->top - Width, Width, Rect->bottom - Rect->top + Width * 2, ROP) &&
        PatBlt(DC, Rect->right, Rect->top - Width, Width, Rect->bottom - Rect->top + Width * 2, ROP) &&
        PatBlt(DC, Rect->left, Rect->top - Width, Rect->right - Rect->left, Width, ROP) &&
        PatBlt(DC, Rect->left, Rect->bottom, Rect->right - Rect->left, Width, ROP)
        :
        PatBlt(DC, Rect->left, Rect->top, -Width, Rect->bottom - Rect->top, ROP) &&
        PatBlt(DC, Rect->right + Width, Rect->top, -Width, Rect->bottom - Rect->top, ROP) &&
        PatBlt(DC, Rect->left - Width, Rect->top, Rect->right - Rect->left + Width * 2, -Width, ROP) &&
        PatBlt(DC, Rect->left - Width, Rect->bottom + Width, Rect->right - Rect->left + Width * 2, -Width, ROP);
}

#pragma endregion

EXTERN_C_END
