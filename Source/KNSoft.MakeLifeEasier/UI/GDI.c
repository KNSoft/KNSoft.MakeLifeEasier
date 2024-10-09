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
