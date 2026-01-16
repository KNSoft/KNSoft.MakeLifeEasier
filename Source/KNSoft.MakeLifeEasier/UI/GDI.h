#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

FORCEINLINE
COLORREF
UI_InverseRGB(
    _In_ COLORREF Color)
{
    return ~(Color) & 0xFFFFFF;
}

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
UI_FillSolidRect(
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
UI_DrawFrameRect(
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

#pragma region Bitmap and Icon

FORCEINLINE
VOID
UI_InitBitmapInfo(
    _Out_ PBITMAPINFOHEADER InfoHeader,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ USHORT BitsPerPixel)
{
    InfoHeader->biSize = sizeof(BITMAPINFOHEADER);
    InfoHeader->biWidth = Width;
    InfoHeader->biHeight = Height;
    InfoHeader->biPlanes = 1;
    InfoHeader->biBitCount = BitsPerPixel;
    InfoHeader->biCompression = BI_RGB;
    InfoHeader->biSizeImage = Width * Height;
    InfoHeader->biXPelsPerMeter =
        InfoHeader->biYPelsPerMeter =
        InfoHeader->biClrUsed =
        InfoHeader->biClrImportant = 0;
}

/* Create a RGB DIB */
FORCEINLINE
_Success_(return != NULL)
HBITMAP
UI_CreateBitmap(
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ USHORT BitsPerPixel,
    _Outptr_result_bytebuffer_(_Inexpressible_(GDI_WIDTHBYTES(Width * BitsPerPixel) * Height)) PVOID * Bits)
{
    BITMAPINFO Info;

    UI_InitBitmapInfo(&Info.bmiHeader, Width, Height, BitsPerPixel);
    return CreateDIBSection(NULL, &Info, DIB_RGB_COLORS, Bits, NULL, 0);
}

/// <summary>
/// Writes bitmap (DDB) data to buffer
/// <see href="https://docs.microsoft.com/en-US/windows/win32/gdi/storing-an-image">Storing an Image - MSDN</see>
/// <see href="https://docs.microsoft.com/en-us/windows/win32/gdi/bitmap-storage">Bitmap Storage - MSDN</see>
/// </summary>
MLE_API
W32ERROR
NTAPI
UI_WriteBitmapFileData(
    _In_ HDC DC,
    _In_ HBITMAP Bitmap,
    _Out_writes_bytes_opt_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_opt_ PULONG ReturnLength);

MLE_API
_Success_(return != NULL)
HBITMAP
NTAPI
UI_CreateBitmapFromIcon(
    _In_ HICON Icon,
    _In_opt_ INT CX,
    _In_opt_ INT CY);

FORCEINLINE
VOID
UI_SetBitmapBitsAlpha(
    _Inout_updates_(BitCount) RGBQUAD* Bits,
    _In_ ULONG BitCount,
    _In_ BYTE Alpha)
{
    ULONG i;

    for (i = 0; i < BitCount; i++)
    {
        Bits[i].rgbReserved = Alpha;
    }
}

#pragma endregion

EXTERN_C_END
