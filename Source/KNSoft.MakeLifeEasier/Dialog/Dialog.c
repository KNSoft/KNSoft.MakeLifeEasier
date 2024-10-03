#include "../MakeLifeEasier.inl"

#pragma pack(push, 1)

typedef struct _DLGTEMPLATEEX_FONTINFO
{
    WORD    pointsize;
    WORD    weight;
    BYTE    italic;
    BYTE    charset;
    WCHAR   typeface[];
} DLGTEMPLATEEX_FONTINFO, *PDLGTEMPLATEEX_FONTINFO;

#pragma pack(pop)

W32ERROR
NTAPI
Dlg_SetTemplateFont(
    _In_ PDLGTEMPLATEEX Template,
    _In_ ULONG TemplateSize,
    _In_opt_ PDLG_SETTEMPLATEFONT FontInfoToSet,
    _Out_opt_ _When_(FontInfoToSet != NULL, _Notnull_) PDLGTEMPLATEEX NewTemplate,
    _Out_opt_ PULONG NewTemplateSize)
{
    PBYTE VarSizePtr;
    PVOID EndPtr;
    PDLGTEMPLATEEX_FONTINFO OrgFontInfoPtr, NewFontInfoPtr;
    WORD SzOrOrdFlag;
    ULONG i;
    SIZE_T s;

    if (Template->dlgVer != 1 || Template->signature != 0xFFFF)
    {
        return ERROR_INVALID_PARAMETER;
    }
    VarSizePtr = Add2Ptr(Template, sizeof(DLGTEMPLATEEX));
    EndPtr = Add2Ptr(Template, TemplateSize);

    /* menu and windowClass */
    for (i = 0; i < 2; i++)
    {
        if (Add2Ptr(VarSizePtr, sizeof(WORD)) > EndPtr)
        {
            return ERROR_INVALID_PARAMETER;
        }
        SzOrOrdFlag = *(PWORD)VarSizePtr;
        if (SzOrOrdFlag == 0x0000)
        {
            VarSizePtr += 2;
        } else if (SzOrOrdFlag == 0xFFFF)
        {
            VarSizePtr += 4;
        } else
        {
            VarSizePtr += 2;
            i = PtrOffset(VarSizePtr, EndPtr) / sizeof(WCHAR);
            s = wcsnlen((PCWSTR)VarSizePtr, i);
            if (s == i)
            {
                return ERROR_INVALID_PARAMETER;
            }
            VarSizePtr += (s + 1) * sizeof(WCHAR);
        }
    }

    /* title */
    if (Add2Ptr(VarSizePtr, sizeof(WORD)) > EndPtr)
    {
        return ERROR_INVALID_PARAMETER;
    }
    SzOrOrdFlag = *(PWORD)VarSizePtr;
    if (SzOrOrdFlag == 0x0000)
    {
        VarSizePtr += 2;
    } else
    {
        VarSizePtr += (wcsnlen((PCWSTR)VarSizePtr, PtrOffset(VarSizePtr, EndPtr) / sizeof(WCHAR)) + 1) * sizeof(WCHAR);
    }

    if (Template->style & (DS_SETFONT | DS_SHELLFONT))
    {
        if (Add2Ptr(VarSizePtr, sizeof(DLGTEMPLATEEX_FONTINFO)) > EndPtr)
        {
            return ERROR_INVALID_PARAMETER;
        }
        OrgFontInfoPtr = (PDLGTEMPLATEEX_FONTINFO)VarSizePtr;
    } else
    {
        OrgFontInfoPtr = NULL;
    }

    return ERROR_UNIDENTIFIED_ERROR;
}
