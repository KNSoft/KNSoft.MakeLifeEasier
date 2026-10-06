#include "../MakeLifeEasier.inl"

#include <UserEnv.h>

#pragma comment(lib, "UserEnv.lib")

W32ERROR
NTAPI
PS_CreateProcess(
    _In_opt_ HANDLE TokenHandle,
    _In_opt_ PCWSTR ApplicationName,
    _Inout_opt_ PWSTR CommandLine,
    _In_ LOGICAL InheritHandles,
    _In_ ULONG CreationFlags,
    _In_opt_ PCWSTR CurrentDirectory,
    _In_opt_ LPSTARTUPINFOW StartupInfo,
    _Out_ LPPROCESS_INFORMATION ProcessInformation)
{
    W32ERROR Ret;
    PVOID Environment = NULL;
    STARTUPINFOW si, *psi;

    if (TokenHandle != NULL)
    {
        if (!CreateEnvironmentBlock(&Environment, TokenHandle, FALSE))
        {
            return Err_GetLastError();
        }
        CreationFlags |= CREATE_UNICODE_ENVIRONMENT;
    }
    if (StartupInfo != NULL)
    {
        psi = StartupInfo;
    } else
    {
        RtlZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        psi = &si;
    }
    Ret = CreateProcessInternalW(TokenHandle,
                                 ApplicationName,
                                 CommandLine,
                                 NULL,
                                 NULL,
                                 InheritHandles,
                                 CreationFlags,
                                 Environment,
                                 CurrentDirectory,
                                 psi,
                                 ProcessInformation,
                                 NULL) ? ERROR_SUCCESS : Err_GetLastError();
    if (Environment != NULL)
    {
        DestroyEnvironmentBlock(Environment);
    }
    return Ret;
}
