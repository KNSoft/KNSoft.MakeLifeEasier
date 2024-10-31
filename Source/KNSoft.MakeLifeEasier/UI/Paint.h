#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

#pragma region Buffered Paint

typedef struct _UI_BUFFEREDPAINT
{
    PAINTSTRUCT PaintStruct;    // Native PAINTSTRUCT structure from "BeginPaint"
    HDC         DC;             // Compatible memory DC
    HBITMAP     Bitmap;         // Compatible memory bitmap
    RECT        Rect;           // RECT of window client area
} UI_BUFFEREDPAINT, *PUI_BUFFEREDPAINT;

MLE_API
W32ERROR
NTAPI
UI_BeginBufferedPaint(
    _In_ HWND Window,
    _Out_ PUI_BUFFEREDPAINT Paint);

MLE_API
W32ERROR
NTAPI
UI_EndBufferedPaint(
    _In_ HWND Window,
    _In_ PUI_BUFFEREDPAINT Paint);

#pragma endregion

EXTERN_C_END
