#include "../MakeLifeEasier.inl"

W32ERROR
NTAPI
Sys_GetSessionToken(
    _In_ DWORD SessionId,
    _Out_ PHANDLE TokenHandle)
{
    WINSTATIONUSERTOKEN UserToken;
    ULONG ReturnLength;

    *TokenHandle = NULL;
    UserToken.ProcessId = NtCurrentProcessId();
    UserToken.ThreadId = NtCurrentThreadId();
    if (!WinStationQueryInformationW(SERVERNAME_CURRENT,
                                     SessionId,
                                     WinStationUserToken,
                                     &UserToken,
                                     sizeof(UserToken),
                                     &ReturnLength))
    {
        return Err_GetLastError();
    }
    *TokenHandle = UserToken.UserToken;
    return ERROR_SUCCESS;
}
