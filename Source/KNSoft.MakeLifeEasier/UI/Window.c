#include "../MakeLifeEasier.inl"

VOID
NTAPI
UI_GetScreenPos(
    _Out_opt_ PPOINT Point,
    _Out_opt_ PSIZE Size)
{
    if (Point)
    {
        Point->x = GetSystemMetrics(SM_XVIRTUALSCREEN);
        Point->y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    }
    if (Size)
    {
        Size->cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        Size->cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    }
}
