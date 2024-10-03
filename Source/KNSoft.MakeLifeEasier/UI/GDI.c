#include "../MakeLifeEasier.inl"

#pragma region Font

/* ENUMLOGFONTEXDVW->elfEnumLogfontEx.elfLogFont should be filled by the caller */
static
VOID
UI_InitInternalFontInfo(
    _Out_ PENUMLOGFONTEXDVW FontInfo)
{
    FontInfo->elfEnumLogfontEx.elfFullName[0] = FontInfo->elfEnumLogfontEx.elfStyle[0] = FontInfo->elfEnumLogfontEx.elfScript[0] = '\0';
    FontInfo->elfDesignVector.dvReserved = STAMP_DESIGNVECTOR;
    FontInfo->elfDesignVector.dvNumAxes = 0;
}

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
    _In_opt_z_ PCWSTR Name)
{
    PLOGFONTW pFontInfo;

    UI_InitInternalFontInfo(FontInfo);
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
        pFontInfo->lfFaceName[0] = '\0';
        return TRUE;
    } else
    {
        return Str_CopyW(pFontInfo->lfFaceName, Name) < ARRAYSIZE(pFontInfo->lfFaceName);
    }
}

_Success_(return != FALSE)
LOGICAL
NTAPI
UI_GetDefaultFont(
    _Out_ PENUMLOGFONTEXDVW FontInfo,
    _In_opt_ LONG NewHeight)
{
    NONCLIENTMETRICSW ncm;

    ncm.cbSize = sizeof(ncm);
    if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
    {
        UI_InitInternalFontInfo(FontInfo);
        memcpy(&FontInfo->elfEnumLogfontEx.elfLogFont,
               &ncm.lfMessageFont,
               sizeof(FontInfo->elfEnumLogfontEx.elfLogFont));
        if (NewHeight)
        {
            FontInfo->elfEnumLogfontEx.elfLogFont.lfHeight = NewHeight;
            FontInfo->elfEnumLogfontEx.elfLogFont.lfWidth = 0;
        }
        return TRUE;
    }

    return FALSE;
}

_Success_(return != NULL)
LOGICAL
NTAPI
UI_GetFontInfo(
    _In_ HFONT Font,
    _Out_ PENUMLOGFONTEXDVW FontInfo)
{
    UI_InitInternalFontInfo(FontInfo);
    return GetObjectW(Font, sizeof(FontInfo->elfEnumLogfontEx.elfLogFont), &FontInfo->elfEnumLogfontEx.elfLogFont) > 0;
}

#pragma endregion
