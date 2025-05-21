#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

#pragma region Token

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

#pragma endregion

EXTERN_C_END
