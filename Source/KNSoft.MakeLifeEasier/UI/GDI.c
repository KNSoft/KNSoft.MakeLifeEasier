#include "../MakeLifeEasier.inl"

#pragma region Font

/* ENUMLOGFONTEXDVW->elfEnumLogfontEx.elfLogFont should be filled by the caller */
static
VOID
UI_InitFontInfo_Impl(
    _Out_ PENUMLOGFONTEXDVW FontInfo)
{
    FontInfo->elfEnumLogfontEx.elfFullName[0] =
        FontInfo->elfEnumLogfontEx.elfStyle[0] =
        FontInfo->elfEnumLogfontEx.elfScript[0] = UNICODE_NULL;
    FontInfo->elfDesignVector.dvReserved = STAMP_DESIGNVECTOR;
    FontInfo->elfDesignVector.dvNumAxes = 0;
}

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
    _In_opt_ PCWSTR Name)
{
    PLOGFONTW pFontInfo;

    UI_InitFontInfo_Impl(FontInfo);
    pFontInfo = &FontInfo->elfEnumLogfontEx.elfLogFont;
    pFontInfo->lfHeight = Height;
    pFontInfo->lfWidth = Width;
    pFontInfo->lfEscapement = Escapement;
    pFontInfo->lfOrientation = Orientation;
    pFontInfo->lfWeight = Weight;
    pFontInfo->lfItalic = Italic;
    pFontInfo->lfUnderline = Underline;
    pFontInfo->lfStrikeOut = StrikeOut;
    pFontInfo->lfCharSet = CharSet;
    pFontInfo->lfOutPrecision = OutPrecision;
    pFontInfo->lfClipPrecision = ClipPrecision;
    pFontInfo->lfQuality = Quality;
    pFontInfo->lfPitchAndFamily = PitchAndFamily;
    if (Name == NULL)
    {
        pFontInfo->lfFaceName[0] = UNICODE_NULL;
    } else if (Str_CopyW(pFontInfo->lfFaceName, Name) >= ARRAYSIZE(pFontInfo->lfFaceName))
    {
        return ERROR_INVALID_PARAMETER;
    }
    return ERROR_SUCCESS;
}

W32ERROR
NTAPI
UI_GetDefaultFont(
    _Out_ PENUMLOGFONTEXDVW FontInfo,
    _In_opt_ LONG NewHeight)
{
    NONCLIENTMETRICSW ncm;

    ncm.cbSize = sizeof(ncm);
    if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
    {
        return NtGetLastError();
    }

    UI_InitFontInfo_Impl(FontInfo);
    memcpy(&FontInfo->elfEnumLogfontEx.elfLogFont,
           &ncm.lfMessageFont,
           sizeof(FontInfo->elfEnumLogfontEx.elfLogFont));
    if (NewHeight)
    {
        FontInfo->elfEnumLogfontEx.elfLogFont.lfHeight = NewHeight;
        FontInfo->elfEnumLogfontEx.elfLogFont.lfWidth = 0;
    }
    return ERROR_SUCCESS;
}

W32ERROR
NTAPI
UI_CreateDefaultFont(
    _Out_ HFONT* Font,
    _In_opt_ LONG NewHeight)
{
    W32ERROR Ret;
    ENUMLOGFONTEXDVW FontInfo;
    HFONT hFont;

    Ret = UI_GetDefaultFont(&FontInfo, NewHeight);
    if (Ret != ERROR_SUCCESS)
    {
        return Ret;
    }

    hFont = CreateFontIndirectExW(&FontInfo);
    if (hFont == NULL)
    {
        return NtGetLastError();
    } else
    {
        *Font = hFont;
        return ERROR_SUCCESS;
    }
}

_Success_(return != FALSE)
LOGICAL
NTAPI
UI_GetFontInfo(
    _In_ HFONT Font,
    _Out_ PENUMLOGFONTEXDVW FontInfo)
{
    UI_InitFontInfo_Impl(FontInfo);
    return GetObjectW(Font, sizeof(FontInfo->elfEnumLogfontEx.elfLogFont), &FontInfo->elfEnumLogfontEx.elfLogFont) > 0;
}

#pragma endregion

#pragma region Bitmap and Icon

W32ERROR
NTAPI
UI_WriteBitmapFileData(
    _In_ HDC DC,
    _In_ HBITMAP Bitmap,
    _Out_writes_bytes_opt_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_opt_ PULONG ReturnLength)
{
    W32ERROR ret;
    BITMAP bmp;
    ULONG cClrBits;
    PBITMAPFILEHEADER pbmfh;
    PBITMAPINFO pbmi;
    DWORD dwClrItem, dwClrSize;
    UINT uHeadersSize, uFileSize, uImageSize;
    PVOID pBits;

    // Get BITMAP structure
    if (GetObjectW(Bitmap, sizeof(BITMAP), &bmp) != sizeof(BITMAP))
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Calculate count of bits and color table items
    cClrBits = bmp.bmPlanes * bmp.bmBitsPixel;
    if (cClrBits == 1)
    {
        cClrBits = 1;
    } else if (cClrBits <= 4)
    {
        cClrBits = 4;
    } else if (cClrBits <= 8)
    {
        cClrBits = 8;
    } else if (cClrBits <= 16)
    {
        cClrBits = 16;
    } else if (cClrBits <= 24)
    {
        cClrBits = 24;
    } else
    {
        cClrBits = 32;
    }
    if (cClrBits < 24)
    {
        dwClrItem = 1 << cClrBits;
        dwClrSize = dwClrItem * sizeof(RGBQUAD);
    } else
    {
        dwClrItem = dwClrSize = 0;
    }

    // Calculate size of image and file
    uHeadersSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwClrSize;
    uImageSize = ROUND_TO_SIZE(bmp.bmWidth * cClrBits, 32) / 8 * bmp.bmHeight;
    uFileSize = uHeadersSize + uImageSize;

    // Write bitmap
    if (Buffer != NULL)
    {
        if (BufferSize < uFileSize)
        {
            ret = ERROR_INSUFFICIENT_BUFFER;
            goto _Exit;
        }
        pbmfh = (PBITMAPFILEHEADER)Buffer;
        pbmfh->bfType = 'MB';
        pbmfh->bfOffBits = uHeadersSize;
        pbmfh->bfSize = uFileSize;
        pbmfh->bfReserved1 = pbmfh->bfReserved2 = 0;
        pbmi = Add2Ptr(pbmfh, sizeof(*pbmfh));
        pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        pbmi->bmiHeader.biWidth = bmp.bmWidth;
        pbmi->bmiHeader.biHeight = bmp.bmHeight;
        pbmi->bmiHeader.biPlanes = bmp.bmPlanes;
        pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel;
        pbmi->bmiHeader.biSizeImage = uImageSize;
        pbmi->bmiHeader.biClrUsed = dwClrItem;
        pbmi->bmiHeader.biCompression = BI_RGB;
        pbmi->bmiHeader.biXPelsPerMeter = pbmi->bmiHeader.biYPelsPerMeter = pbmi->bmiHeader.biClrImportant = 0;
        pBits = Add2Ptr(pbmi, pbmi->bmiHeader.biSize + (DWORD_PTR)dwClrSize);
        if (!GetDIBits(DC, Bitmap, 0, bmp.bmHeight, pBits, pbmi, DIB_RGB_COLORS))
        {
            return ERROR_INVALID_PARAMETER;
        }
    }
    ret = ERROR_SUCCESS;

_Exit:
    if (ReturnLength != NULL)
    {
        *ReturnLength = uFileSize;
    }
    return ret;
}

_Success_(return != NULL)
HBITMAP
NTAPI
UI_CreateBitmapFromIcon(
    _In_ HICON Icon,
    _In_opt_ INT CX,
    _In_opt_ INT CY)
{
    ICONINFO Info;
    BITMAP MaskBitmapInfo;
    INT RealHeight;
    HDC ScreenDC, MemDC;
    HBITMAP Ret, Bitmap, OldBitmap;
    RECT Rect;

    if (!GetIconInfo(Icon, &Info))
    {
        return NULL;
    }
    Ret = NULL;
    if (GetObjectW(Info.hbmMask, sizeof(MaskBitmapInfo), &MaskBitmapInfo) == 0)
    {
        goto _Exit_0;
    }
    RealHeight = MaskBitmapInfo.bmHeight;
    if (Info.hbmColor == NULL)
    {
        RealHeight /= 2;
    }
    Rect.right = CX != 0 ? CX : MaskBitmapInfo.bmWidth;
    Rect.bottom = CY != 0 ? CY : RealHeight;

    ScreenDC = GetDC(NULL);
    if (ScreenDC == NULL)
    {
        goto _Exit_0;
    }
    MemDC = CreateCompatibleDC(ScreenDC);
    if (MemDC == NULL)
    {
        goto _Exit_1;
    }
    Bitmap = CreateCompatibleBitmap(ScreenDC, Rect.right, Rect.bottom);
    if (Bitmap == NULL)
    {
        goto _Exit_2;
    }
    OldBitmap = SelectObject(MemDC, Bitmap);
    if (OldBitmap == NULL)
    {
        goto _Exit_3;
    }
    Rect.left = Rect.top = 0;
    SetBkColor(MemDC, RGB(255, 255, 255));
    ExtTextOutW(MemDC, 0, 0, ETO_OPAQUE, &Rect, NULL, 0, NULL);
    if (DrawIconEx(MemDC, 0, 0, Icon, Rect.right, Rect.bottom, 0, NULL, DI_NORMAL))
    {
        Ret = Bitmap;
    }
    SelectObject(MemDC, OldBitmap);
    if (Ret != NULL)
    {
        goto _Exit_2;
    }

_Exit_3:
    DeleteObject(Bitmap);
_Exit_2:
    DeleteDC(MemDC);
_Exit_1:
    ReleaseDC(NULL, ScreenDC);
_Exit_0:
    DeleteObject(Info.hbmMask);
    if (Info.hbmColor != NULL)
    {
        DeleteObject(Info.hbmColor);
    }
    return Ret;
}

#pragma endregion
