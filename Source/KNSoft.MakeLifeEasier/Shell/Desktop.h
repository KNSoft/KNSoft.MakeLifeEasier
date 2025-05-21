#pragma once

#include "../MakeLifeEasier.h"

/*
 * Reversed from the latest Win11 Explorer.exe.
 *
 * v_hwndDesktop = GetShellWindow();
 * CTrayStatic c_ctrayStatic;
 * c_ctrayStatic + 8 = Shell_TrayWnd = GetAncestor(GetTaskmanWindow(), GA_ROOT);
 */

EXTERN_C_START

FORCEINLINE
W32ERROR
Shell_GetExplorerProcessId(
    _Out_ PULONG ProcessId)
{
    HWND ShellWindow;

    ShellWindow = GetShellWindow();
    if (ShellWindow == NULL)
    {
        return ERROR_NOT_FOUND;
    }
    return GetWindowThreadProcessId(ShellWindow, ProcessId) != 0 ? ERROR_SUCCESS : Err_GetLastError();
}

FORCEINLINE
HWND
Shell_GetTrayWindow(VOID)
{
    HWND Window = GetTaskmanWindow();
    return Window != NULL ? GetAncestor(Window, GA_ROOT) : NULL;
}

MLE_API
W32ERROR
NTAPI
Shell_QuitShellWindow(
    _In_ HWND ShellWindow);

MLE_API
W32ERROR
NTAPI
Shell_ExitExplorerEx(
    _In_opt_ ULONG Timeout,
    _Out_opt_ PULONG OldProcessId);

MLE_API
W32ERROR
NTAPI
Shell_StartExplorerEx(
    _In_opt_ ULONG Timeout,
    _In_opt_ ULONG OldProcessId);

EXTERN_C_END
