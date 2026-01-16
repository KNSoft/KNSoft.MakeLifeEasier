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
    _In_opt_ PCWSTR CurrentDirectory,
    _In_opt_ LPSTARTUPINFOW StartupInfo,
    _Out_ LPPROCESS_INFORMATION ProcessInformation)
{
    W32ERROR Ret;
    PVOID Environment;
    STARTUPINFOW si, *psi;
    DWORD CreationFlags = NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_DEFAULT_ERROR_MODE;

    if (TokenHandle != NULL)
    {
        if (CreateEnvironmentBlock(&Environment, TokenHandle, FALSE))
        {
            CreationFlags |= CREATE_UNICODE_ENVIRONMENT;
        }
    } else
    {
        Environment = NULL;
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
    if (Environment)
    {
        DestroyEnvironmentBlock(Environment);
    }
    return Ret;
}
