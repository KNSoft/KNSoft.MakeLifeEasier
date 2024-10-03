#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

#define Dlg_MsgBox(Owner, Text, Title, Type) MessageBoxTimeoutW(Owner, Text, Title, Type, 0, -1)

MLE_API
W32ERROR
NTAPI
Dlg_RectEditor(
    _In_opt_ HWND Owner,
    _Inout_ PRECT Rect);

#define DLG_SETTEMPLATEFONT_FLAG_SET_POINTSIZE 0x1
#define DLG_SETTEMPLATEFONT_FLAG_SET_WEIGHT 0x2
#define DLG_SETTEMPLATEFONT_FLAG_SET_ITALIC 0x4
#define DLG_SETTEMPLATEFONT_FLAG_SET_CHARSET 0x8
#define DLG_SETTEMPLATEFONT_FLAG_SET_TYPEFACE 0x10

typedef struct _DLG_SETTEMPLATEFONT
{
    ULONG   Flags;      // DLG_SETTEMPLATEFONT_FLAG_*
    WORD    PointSize;
    WORD    Weight;     // lfWeight
    BYTE    Italic;     // lfItalic
    BYTE    Charset;    // lfcharset
    PCWSTR  Typeface;   // NULL for DS_SHELLFONT
} DLG_SETTEMPLATEFONT, *PDLG_SETTEMPLATEFONT;

EXTERN_C_END
