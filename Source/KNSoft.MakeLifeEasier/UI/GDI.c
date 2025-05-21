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
        return Err_GetLastError();
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
        return Err_GetLastError();
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

/* See also: https://learn.microsoft.com/en-us/previous-versions/bb757020(v=msdn.10) */
_Success_(return != NULL)
HBITMAP
NTAPI
UI_CreateBitmapFromIcon(
    _In_ HICON Icon,
    _In_opt_ INT CX,
    _In_opt_ INT CY)
{
    ICONINFO IconInfo;
    BITMAP MaskBitmapInfo;
    BITMAPINFO MaskBitmapInfo2;
    INT RealHeight;
    HDC MemDC, BufferDC;
    PVOID BitmapBits, MaskBitmapBits;
    HBITMAP Ret, Bitmap, OldBitmap;
    RECT Rect;
    HPAINTBUFFER PaintBuffer;
    BLENDFUNCTION AlphaBlendFunc;
    BP_PAINTPARAMS PaintParam;
    RGBQUAD *RGBQuad, *ARGB;
    INT CXRow, RowNum, ColNum;
    LOGICAL HasAlphaChannel;
    PUINT32 ARGBMask;

    /* Get icon bitmap and size */

    if (!GetIconInfo(Icon, &IconInfo))
    {
        return NULL;
    }
    Ret = NULL;
    if (GetObjectW(IconInfo.hbmMask, sizeof(MaskBitmapInfo), &MaskBitmapInfo) == 0)
    {
        goto _Exit_0;
    }
    RealHeight = MaskBitmapInfo.bmHeight;
    if (IconInfo.hbmColor == NULL)
    {
        RealHeight /= 2;
    }
    Rect.right = CX != 0 ? CX : MaskBitmapInfo.bmWidth;
    Rect.bottom = CY != 0 ? CY : RealHeight;

    /* Create 32-bit DIB and prepare memory DC */

    MemDC = CreateCompatibleDC(NULL);
    if (MemDC == NULL)
    {
        goto _Exit_0;
    }
    Bitmap = UI_CreateBitmap(Rect.right, Rect.bottom, 32, &BitmapBits);
    if (Bitmap == NULL)
    {
        goto _Exit_1;
    }
    OldBitmap = SelectObject(MemDC, Bitmap);
    if (OldBitmap == NULL)
    {
        goto _Exit_2;
    }

    /* Draw icon to memory DC with alpha blend */

    Rect.top = Rect.left = 0;
    AlphaBlendFunc.BlendOp = AC_SRC_OVER;
    AlphaBlendFunc.BlendFlags = 0;
    AlphaBlendFunc.SourceConstantAlpha = 255;
    AlphaBlendFunc.AlphaFormat = AC_SRC_ALPHA;
    PaintParam.cbSize = sizeof(PaintParam);
    PaintParam.dwFlags = BPPF_ERASE;
    PaintParam.prcExclude = NULL;
    PaintParam.pBlendFunction = &AlphaBlendFunc;

    PaintBuffer = BeginBufferedPaint(MemDC, &Rect, BPBF_DIB, &PaintParam, &BufferDC);
    if (PaintBuffer == NULL)
    {
        goto _Fallback_Opaque_Draw_0;
    }
    if (!DrawIconEx(BufferDC, 0, 0, Icon, Rect.right, Rect.bottom, 0, NULL, DI_NORMAL))
    {
        goto _Fallback_Opaque_Draw_1;
    }
    if (FAILED(GetBufferedPaintBits(PaintBuffer, &RGBQuad, &CXRow)))
    {
        goto _Fallback_Opaque_Draw_1;
    }

    /* Find alpha channel in the icon */
    HasAlphaChannel = FALSE;
    CXRow -= Rect.bottom;
    ARGB = RGBQuad;
    for (ColNum = Rect.right; ColNum > 0; ColNum--)
    {
        for (RowNum = Rect.bottom; RowNum > 0; RowNum--)
        {
            if (ARGB->rgbReserved != 0)
            {
                HasAlphaChannel = TRUE;
                goto _End_Find_Alpha;
            }
            ARGB++;
        }
        ARGB += CXRow;
    }
_End_Find_Alpha:

    /* Mix alpha if the icon has no alpha channel */
    if (HasAlphaChannel)
    {
        goto _Skip_Alpha_Mix;
    }
    MaskBitmapBits = Mem_Alloc(Rect.right * sizeof(RGBQUAD) * Rect.bottom);
    if (MaskBitmapBits == NULL)
    {
        goto _Fallback_Opaque_Draw_1;
    }
    UI_InitBitmapInfo(&MaskBitmapInfo2.bmiHeader, Rect.right, Rect.bottom, 32);
    if (GetDIBits(MemDC,
                  IconInfo.hbmMask,
                  0,
                  Rect.bottom,
                  MaskBitmapBits,
                  &MaskBitmapInfo2,
                  DIB_RGB_COLORS) != Rect.bottom)
    {
        goto _Fallback_Opaque_Draw_2;
    }

    ARGB = RGBQuad;
    ARGBMask = (PUINT32)MaskBitmapBits;
    for (ColNum = Rect.right; ColNum > 0; ColNum--)
    {
        for (RowNum = Rect.bottom; RowNum > 0; RowNum--)
        {
            if (*ARGBMask++)
            {
                ARGB->rgbBlue = ARGB->rgbGreen = ARGB->rgbRed = ARGB->rgbReserved = 0;
            } else
            {
                ARGB->rgbReserved = 255;
            }
            ARGB++;
        }
        ARGB += CXRow;
    }
    Mem_Free(MaskBitmapBits);

_Skip_Alpha_Mix:
    if (FAILED(EndBufferedPaint(PaintBuffer, TRUE)))
    {
        goto _Fallback_Opaque_Draw_0;
    }
    Ret = Bitmap;
    goto _End_Draw;

    /* Fallback to draw icon without alpha */
_Fallback_Opaque_Draw_2:
    Mem_Free(MaskBitmapBits);
_Fallback_Opaque_Draw_1:
    EndBufferedPaint(PaintBuffer, TRUE);
_Fallback_Opaque_Draw_0:
    SetBkColor(MemDC, RGB(255, 255, 255));
    ExtTextOutW(MemDC, 0, 0, ETO_OPAQUE, &Rect, NULL, 0, NULL);
    if (DrawIconEx(MemDC, 0, 0, Icon, Rect.right, Rect.bottom, 0, NULL, DI_NORMAL))
    {
        Ret = Bitmap;
    }

_End_Draw:
    SelectObject(MemDC, OldBitmap);
    if (Ret != NULL)
    {
        goto _Exit_1;
    }

_Exit_2:
    DeleteObject(Bitmap);
_Exit_1:
    DeleteDC(MemDC);
_Exit_0:
    DeleteObject(IconInfo.hbmMask);
    if (IconInfo.hbmColor != NULL)
    {
        DeleteObject(IconInfo.hbmColor);
    }
    return Ret;
}

#pragma endregion
