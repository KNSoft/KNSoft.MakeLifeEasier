#include "Startup.inl"

#include <WinBase.h>

static void CRT_Common_EXE_Main()
{
    NTSTATUS Status;

    Status = CRT_Startup_Init();
    if (!NT_SUCCESS(Status))
    {
        goto _exit;
    }

    _Analysis_assume_(__wargv != NULL);
    _Analysis_assume_(__argv != NULL);
    _Analysis_assume_(_wcmdln != NULL);
    _Analysis_assume_(_acmdln != NULL);

#if defined(_NTASSASSIN_CRT_STARTUP_EXEMAIN)

    Status = ExeMain();
#elif defined(_NTASSASSIN_CRT_STARTUP_WMAIN)

    Startup_InitCmdlineArgW();
    Status = wmain(__argc, __wargv, NULL);
    Startup_FreeCmdlineArgW();
#elif defined(_NTASSASSIN_CRT_STARTUP_MAIN)

    Startup_InitCmdlineArgA();
    Status = main(__argc, __argv, NULL);
    Startup_FreeCmdlineArgA();
#elif defined(_NTASSASSIN_CRT_STARTUP_WWINMAIN)

    Startup_InitCmdlineW();
    Status = wWinMain((HINSTANCE)(&__ImageBase),
                      NULL,
                      _wcmdln,
                      NtCurrentPeb()->ProcessParameters->ShowWindowFlags);
#elif defined(_NTASSASSIN_CRT_STARTUP_WINMAIN)

    Startup_InitCmdlineA();
    Status = WinMain((HINSTANCE)(&__ImageBase),
                     NULL,
                     _acmdln,
                     NtCurrentPeb()->ProcessParameters->ShowWindowFlags);
    Startup_FreeCmdlineA();
#endif

_exit:
    CRT_Startup_Uninit();
    RtlExitUserProcess(Status);
    __assume(0);
    __fastfail(FAST_FAIL_FATAL_APP_EXIT);
}
