#include "../MakeLifeEasier.inl"

_Success_(return != FALSE)
LOGICAL
NTAPI
UI_BeginBufferedPaint(
    _In_ HWND Window,
    _Out_ PUI_BUFFEREDPAINT Paint)
{
    PAINTSTRUCT ps;
    HDC hdc, hdcMem;
    HBITMAP hbm, hbmOriginal;
    RECT rc;

    hdc = BeginPaint(Window, &ps);
    if (hdc == NULL)
    {
        return FALSE;
    }
    hdcMem = CreateCompatibleDC(hdc);
    if (hdcMem == NULL)
    {
        goto _Exit_0;
    }
    if (!GetClientRect(Window, &rc))
    {
        goto _Exit_1;
    }
    hbm = CreateCompatibleBitmap(ps.hdc, rc.right, rc.bottom);
    if (hbm == NULL)
    {
        goto _Exit_1;
    }
    hbmOriginal = SelectObject(hdcMem, hbm);
    if (hbmOriginal == NULL)
    {
        goto _Exit_2;
    }

    Paint->ps = ps;
    Paint->hdc = hdcMem;
    Paint->hbm = hbm;
    Paint->rc = rc;
    Paint->hbmOriginal = hbmOriginal;
    return TRUE;

_Exit_2:
    DeleteObject(hbm);
_Exit_1:
    DeleteDC(hdcMem);
_Exit_0:
    EndPaint(Window, &ps);
    return FALSE;
}

_Success_(return != FALSE)
LOGICAL
NTAPI
UI_EndBufferedPaint(
    _In_ HWND Window,
    _In_ PUI_BUFFEREDPAINT Paint)
{
    LOGICAL Ret = BitBlt(Paint->ps.hdc,
                         Paint->ps.rcPaint.left,
                         Paint->ps.rcPaint.top,
                         Paint->ps.rcPaint.right - Paint->ps.rcPaint.left,
                         Paint->ps.rcPaint.bottom - Paint->ps.rcPaint.top,
                         Paint->hdc,
                         Paint->ps.rcPaint.left,
                         Paint->ps.rcPaint.top,
                         SRCCOPY);
    SelectObject(Paint->hdc, Paint->hbmOriginal);
    DeleteDC(Paint->hdc);
    DeleteObject(Paint->hbm);
    Ret &= EndPaint(Window, &Paint->ps);
    return Ret;
}
