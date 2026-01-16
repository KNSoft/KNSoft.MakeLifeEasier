#include "../MakeLifeEasier.inl"

W32ERROR
NTAPI
UI_BeginBufferedPaint(
    _In_ HWND Window,
    _Out_ PUI_BUFFEREDPAINT Paint)
{
    W32ERROR Error;
    PAINTSTRUCT ps;
    HDC hDC, hMemDC;
    HBITMAP hMemBitmap;
    RECT rc;

    hDC = BeginPaint(Window, &ps);
    if (hDC == NULL)
    {
        return ERROR_UNIDENTIFIED_ERROR;
    }
    hMemDC = CreateCompatibleDC(hDC);
    if (hMemDC == NULL)
    {
        Error = ERROR_UNIDENTIFIED_ERROR;
        goto _Exit_0;
    }
    if (!GetClientRect(Window, &rc))
    {
        Error = Err_GetLastError();
        goto _Exit_1;
    }
    hMemBitmap = CreateCompatibleBitmap(ps.hdc, rc.right, rc.bottom);
    if (hMemBitmap == NULL)
    {
        Error = ERROR_UNIDENTIFIED_ERROR;
        goto _Exit_1;
    }
    if (SelectObject(hMemDC, hMemBitmap) == NULL)
    {
        Error = ERROR_UNIDENTIFIED_ERROR;
        goto _Exit_2;
    }

    Paint->PaintStruct = ps;
    Paint->DC = hMemDC;
    Paint->Bitmap = hMemBitmap;
    Paint->Rect = rc;
    return ERROR_SUCCESS;

_Exit_2:
    DeleteObject(hMemBitmap);
_Exit_1:
    DeleteDC(hMemDC);
_Exit_0:
    EndPaint(Window, &ps);
    return Error;
}

W32ERROR
NTAPI
UI_EndBufferedPaint(
    _In_ HWND Window,
    _In_ PUI_BUFFEREDPAINT Paint)
{
    W32ERROR Error;

    Error = BitBlt(Paint->PaintStruct.hdc,
                   Paint->PaintStruct.rcPaint.left,
                   Paint->PaintStruct.rcPaint.top,
                   Paint->PaintStruct.rcPaint.right - Paint->PaintStruct.rcPaint.left,
                   Paint->PaintStruct.rcPaint.bottom - Paint->PaintStruct.rcPaint.top,
                   Paint->DC,
                   0,
                   0,
                   SRCCOPY) ? ERROR_SUCCESS : Err_GetLastError();
    DeleteDC(Paint->DC);
    DeleteObject(Paint->Bitmap);
    EndPaint(Window, &Paint->PaintStruct);
    return Error;
}
