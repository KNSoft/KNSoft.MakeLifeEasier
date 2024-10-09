#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

MLE_API
VOID
NTAPI
UI_GetWindowDPI(
    _In_ HWND Window,
    _Out_ PUINT DPIX,
    _Out_ PUINT DPIY);

FORCEINLINE
VOID
UI_DPIScaleInt(
    _Inout_ PINT Value,
    _In_ UINT OldDPI,
    _In_ UINT NewDPI)
{
    *Value = Math_RoundInt((*Value) * ((FLOAT)(NewDPI) / (OldDPI)));
}

FORCEINLINE
VOID
UI_DPIScaleUInt(
    _Inout_ PINT Value,
    _In_ UINT OldDPI,
    _In_ UINT NewDPI)
{
    *Value = Math_RoundUInt((*Value) * ((FLOAT)(NewDPI) / (OldDPI)));
}

MLE_API
VOID
NTAPI
UI_DPIScaleRect(
    _Inout_ PRECT Rect,
    _In_ UINT OldDPIX,
    _In_ UINT NewDPIX,
    _In_ UINT OldDPIY,
    _In_ UINT NewDPIY);

MLE_API
W32ERROR
NTAPI
UI_DPIScaleFont(
    _Inout_ HFONT* Font,
    _In_ UINT OldDPIY,
    _In_ UINT NewDPIY);

MLE_API
W32ERROR
NTAPI
UI_DPIScaleDialogRect(
    _In_ HWND Dialog,
    _In_ UINT OldDPIX,
    _In_ UINT NewDPIX,
    _In_ UINT OldDPIY,
    _In_ UINT NewDPIY,
    _In_ LOGICAL Redraw);

MLE_API
W32ERROR
NTAPI
UI_DPIScaleChildRect(
    _In_ HWND Window,
    _In_ PPOINT ParentScreenOrigin,
    _In_ UINT OldDPIX,
    _In_ UINT NewDPIX,
    _In_ UINT OldDPIY,
    _In_ UINT NewDPIY,
    _In_ LOGICAL Redraw);

MLE_API
W32ERROR
NTAPI
UI_DPIScaleDialog(
    _In_ HWND Dialog,
    _In_ UINT OldDPIX,
    _In_ UINT NewDPIX,
    _In_ UINT OldDPIY,
    _In_ UINT NewDPIY,
    _In_opt_ PRECT SuggestedPos,
    _Inout_opt_ HFONT* Font);

typedef struct _UI_DIALOG_DPI_INFO
{
    DWORD   DPIX;
    DWORD   DPIY;
    HFONT   Font;
} UI_DIALOG_DPI_INFO, *PUI_DIALOG_DPI_INFO;

MLE_API
W32ERROR
NTAPI
UI_InitializeDialogDPIScale(
    _In_ HWND Dialog);

MLE_API
VOID
NTAPI
UI_UninitializeDialogDPIScale(
    _In_ HWND Dialog);

MLE_API
INT_PTR
CALLBACK
UI_DPIScaleDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

MLE_API
_Success_(return != NULL)
PVOID
NTAPI
UI_EnableDPIAwareContext(VOID);

MLE_API
LOGICAL
NTAPI
UI_RestoreDPIAwareContext(
    _In_ PVOID Cookie);

EXTERN_C_END
