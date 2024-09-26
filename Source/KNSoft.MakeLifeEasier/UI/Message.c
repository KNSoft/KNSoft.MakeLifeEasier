#include "../MakeLifeEasier.inl"

W32ERROR
NTAPI
UI_AllowDrop(
    _In_ HWND Window)
{
    return (ChangeWindowMessageFilterEx(Window, WM_DROPFILES, MSGFLT_ADD, NULL) &&
            ChangeWindowMessageFilterEx(Window, WM_COPYGLOBALDATA, MSGFLT_ADD, NULL)) ?
        ERROR_SUCCESS : NtGetLastError();
}
