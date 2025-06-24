#include "../MakeLifeEasier.inl"

/* Reversed from Explorer.exe!CTray::_DoExitExplorer */
W32ERROR
NTAPI
Shell_QuitShellWindow(
    _In_ HWND ShellWindow)
{
    W32ERROR Ret;
    HWND ShellTray;

    Ret = PostMessageW(ShellWindow, WM_QUIT, 0, 0) ? ERROR_SUCCESS : Err_GetLastError();
    ShellTray = Shell_GetTrayWindow();
    if (ShellTray != NULL && PostMessageW(ShellTray, WM_QUIT, 0, 0))
    {
        return ERROR_SUCCESS;
    }
    if (!Ret)
    {
        Ret = EndTask(ShellWindow, FALSE, TRUE) ? ERROR_SUCCESS : Err_GetLastError();
    }
    return Ret;
}

W32ERROR
NTAPI
Shell_ExitExplorerEx(
    _In_opt_ ULONG Timeout,
    _Out_opt_ PULONG OldProcessId)
{
    W32ERROR Ret;
    HWND ShellWindow;
    ULONG ProcessId;
    HANDLE ProcessHandle;
    NTSTATUS Status;

    /* Get shell window and process id */
    ShellWindow = GetShellWindow();
    if (ShellWindow == NULL)
    {
        ProcessId = 0;
        goto _Done;
    }
    Ret = UI_GetWindowThreadProcessId(ShellWindow, NULL, &ProcessId);
    if (Ret != ERROR_SUCCESS)
    {
        return Ret;
    }

    /* Open process handle if need wait it */
    if (Timeout == 0)
    {
        ProcessHandle = NULL;
    } else if (!NT_SUCCESS(PS_OpenProcess(&ProcessHandle, SYNCHRONIZE, ProcessId)))
    {
        goto _Force_Kill;
    }

    /* Quit shell */
    Ret = Shell_QuitShellWindow(ShellWindow);
    if (Ret == ERROR_SUCCESS)
    {
        if (ProcessHandle == NULL)
        {
            goto _Done;
        }
    } else
    {
        if (ProcessHandle != NULL)
        {
            NtClose(ProcessHandle);
        }
        goto _Force_Kill;
    }

    /* Wait shell exit */
    Status = PS_WaitForObject(ProcessHandle, Timeout);
    NtClose(ProcessHandle);
    if (Status == STATUS_WAIT_0)
    {
        goto _Done;
    }

_Force_Kill:
    Status = PS_OpenProcess(&ProcessHandle, PROCESS_TERMINATE, ProcessId);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }
    Status = NtTerminateProcess(ProcessHandle, STATUS_SUCCESS);
    NtClose(ProcessHandle);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

_Done:
    if (OldProcessId != NULL)
    {
        *OldProcessId = ProcessId;
    }
    return ERROR_SUCCESS;
}

#define SHELL_WAIT_EXPLORER_START_UNIT 300

W32ERROR
NTAPI
Shell_StartExplorerEx(
    _In_opt_ ULONG Timeout,
    _In_opt_ ULONG OldProcessId)
{
    W32ERROR Ret;
    HWND ShellWindow;
    WCHAR Path[MAX_PATH];
    ULONG u;

    /* Check if shell window already exists */
    ShellWindow = GetShellWindow();
    if (ShellWindow != NULL)
    {
        Ret = UI_GetWindowThreadProcessId(ShellWindow, NULL, &u);
        if (Ret != ERROR_SUCCESS)
        {
            return Ret;
        } else
        {
            return u != OldProcessId ? ERROR_SUCCESS : ERROR_ALREADY_EXISTS;
        }
    }

    /* Run explorer.exe */
    u = Str_CopyW(Path, SharedUserData->NtSystemRoot);
    if (u + 1 >= ARRAYSIZE(Path))
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }
    Path[u++] = L'\\';
    Str_CopyExW(Path + u, ARRAYSIZE(Path) - u, L"explorer.exe");
    Ret = Shell_Exec(Path, NULL, L"open", SW_SHOWNORMAL, NULL);
    if (Timeout == 0 || Ret != ERROR_SUCCESS)
    {
        return Ret;
    }

    /* Wait shell window start */
    u = 0;
    do
    {
        PS_DelayExec(SHELL_WAIT_EXPLORER_START_UNIT);
        if (GetShellWindow() != NULL && GetTaskmanWindow() != NULL)
        {
            return ERROR_SUCCESS;
        }
        u += SHELL_WAIT_EXPLORER_START_UNIT;
    } while (u < Timeout);
    return ERROR_TIMEOUT;
}
