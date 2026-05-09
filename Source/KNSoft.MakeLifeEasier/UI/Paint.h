#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

#pragma region Buffered Paint

typedef struct _UI_BUFFEREDPAINT
{
    PAINTSTRUCT ps;     // Native PAINTSTRUCT structure from "BeginPaint"
    HDC         hdc;    // Compatible memory DC
    HBITMAP     hbm;    // Compatible memory bitmap
    RECT        rc;     // RECT of window client area

    HBITMAP     hbmOriginal;
} UI_BUFFEREDPAINT, *PUI_BUFFEREDPAINT;

MLE_API
_Success_(return != FALSE)
LOGICAL
NTAPI
UI_BeginBufferedPaint(
    _In_ HWND Window,
    _Out_ PUI_BUFFEREDPAINT Paint);

MLE_API
_Success_(return != FALSE)
LOGICAL
NTAPI
UI_EndBufferedPaint(
    _In_ HWND Window,
    _In_ PUI_BUFFEREDPAINT Paint);

#pragma endregion

EXTERN_C_END
