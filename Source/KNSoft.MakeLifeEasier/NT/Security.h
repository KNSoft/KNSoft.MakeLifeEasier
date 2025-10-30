#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

#pragma region Token

MLE_API EXTERN_C CONST SECURITY_QUALITY_OF_SERVICE NT_DefaultImpersonationSqos;
MLE_API EXTERN_C CONST OBJECT_ATTRIBUTES NT_DefaultImpersonationObjectAttribute;

FORCEINLINE
NTSTATUS
NT_AdjustTokenPrivilege(
    _In_ HANDLE TokenHandle,
    _In_ ULONG Privilege,
    _In_ DWORD Attributes)
{
    TOKEN_PRIVILEGES TokenPrivilege;

    TokenPrivilege.PrivilegeCount = 1;
    TokenPrivilege.Privileges[0].Luid = RtlConvertUlongToLuid(Privilege);
    TokenPrivilege.Privileges[0].Attributes = Attributes;

    return NtAdjustPrivilegesToken(TokenHandle, FALSE, &TokenPrivilege, sizeof(TokenPrivilege), NULL, NULL);
}

MLE_API
NTSTATUS
NTAPI
NT_CreateToken(
    _Out_ PHANDLE TokenHandle,
    _In_ TOKEN_TYPE Type,
    _In_ PSID OwnerSid,
    _In_ PLUID AuthenticationId,
    _In_ PTOKEN_GROUPS Groups,
    _In_ PTOKEN_PRIVILEGES Privileges);

FORCEINLINE
W32ERROR
NT_GetSessionToken(
    _Out_ PHANDLE TokenHandle,
    _In_ DWORD SessionId)
{
    ULONG Length;
    WINSTATIONUSERTOKEN WinStaUserToken;

    WinStaUserToken.ProcessId = NtCurrentProcessId();
    WinStaUserToken.ThreadId = NtCurrentThreadId();
    if (WinStationQueryInformationW(SERVERNAME_CURRENT,
                                    SessionId,
                                    WinStationUserToken,
                                    &WinStaUserToken,
                                    sizeof(WinStaUserToken),
                                    &Length))
    {
        *TokenHandle = WinStaUserToken.UserToken;
        return ERROR_SUCCESS;
    }
    return _Inline_RtlGetLastWin32Error();
}

#pragma endregion

EXTERN_C_END
