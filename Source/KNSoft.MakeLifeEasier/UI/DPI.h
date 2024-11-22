#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

FORCEINLINE
VOID
UI_GetWindowDPI(
    _In_opt_ HWND Window,
    _Out_ PUINT DPIX,
    _Out_ PUINT DPIY)
{
    HDC hDC;

    hDC = GetDC(Window);
    *DPIX = GetDeviceCaps(hDC, LOGPIXELSX);
    *DPIY = GetDeviceCaps(hDC, LOGPIXELSY);
    ReleaseDC(Window, hDC);
}

#pragma region Scale

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

FORCEINLINE
VOID
UI_DPIScaleRect(
    _Inout_ PRECT Rect,
    _In_ UINT OldDPIX,
    _In_ UINT NewDPIX,
    _In_ UINT OldDPIY,
    _In_ UINT NewDPIY)
{
    UI_DPIScaleInt((PINT)&Rect->left, OldDPIX, NewDPIX);
    UI_DPIScaleInt((PINT)&Rect->right, OldDPIX, NewDPIX);
    UI_DPIScaleInt((PINT)&Rect->top, OldDPIY, NewDPIY);
    UI_DPIScaleInt((PINT)&Rect->bottom, OldDPIY, NewDPIY);
}

MLE_API
W32ERROR
NTAPI
UI_DPIScaleFont(
    _Inout_ HFONT* Font,
    _In_ UINT OldDPIY,
    _In_ UINT NewDPIY);

W32ERROR
NTAPI
UI_DPIAdjustWindowRect(
    _In_ HWND Window,
    _Inout_ PRECT Rect,
    _In_ UINT DPI);

#pragma endregion

#pragma region Dialog Auto Scaling

typedef struct _UI_DIALOG_DPI_INFO
{
    DWORD   DPIX;
    DWORD   DPIY;
    HFONT   Font;
} UI_DIALOG_DPI_INFO, *PUI_DIALOG_DPI_INFO;

MLE_API
INT_PTR
CALLBACK
UI_DPIScaleDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

MLE_API
_Success_(return != NULL)
_Ret_maybenull_
PUI_DIALOG_DPI_INFO
NTAPI
UI_GetDialogDPIScaleInfo(
    _In_ HWND Dialog);

#pragma endregion

#pragma region DPI Awareness Context

MLE_API
_Success_(return != NULL)
DPI_AWARENESS_CONTEXT
NTAPI
UI_EnableDPIAwareContext(VOID);

MLE_API
_Success_(return != NULL)
DPI_AWARENESS_CONTEXT
NTAPI
UI_RestoreDPIAwareContext(
    _In_opt_ DPI_AWARENESS_CONTEXT Context);

MLE_API
_Success_(return != NULL)
DPI_AWARENESS_CONTEXT
NTAPI
UI_GetDPIAwareContext(VOID);

MLE_API
DPI_AWARENESS_CONTEXT
NTAPI
UI_GetWindowDPIAwareContext(
    _In_ HWND Window);

MLE_API
LOGICAL
NTAPI
UI_CompareDPIAwareContext(
    _In_ DPI_AWARENESS_CONTEXT Context1,
    _In_ DPI_AWARENESS_CONTEXT Context2);

#pragma endregion

EXTERN_C_END
