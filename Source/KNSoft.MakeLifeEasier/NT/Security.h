#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

#pragma region SID

/// <seealso cref="RtlLengthSid"/>
FORCEINLINE
_Post_satisfies_(return >= 8 && return <= SECURITY_MAX_SID_SIZE)
ULONG
NT_LengthSid(
    _In_ PSID Sid)
{
    return UFIELD_OFFSET(SID, SubAuthority[(Sid)->SubAuthorityCount]);
}

/// <seealso cref="RtlEqualSid"/>
FORCEINLINE
LOGICAL
NT_EqualSid(
    _In_ PSID Sid1,
    _In_ PSID Sid2)
{
    ULONG Length = NT_LengthSid(Sid1);
    return RtlCompareMemory(Sid1, Sid2, Length) == Length;
}

/// <seealso cref="RtlCopySid"/>
FORCEINLINE
NTSTATUS
NT_CopySid(
    _In_ ULONG DestinationSidLength,
    _Out_writes_bytes_(DestinationSidLength) PSID DestinationSid,
    _In_ PSID SourceSid)
{
    ULONG Length = NT_LengthSid(SourceSid);

    if (DestinationSidLength < Length)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    memcpy(DestinationSid, SourceSid, Length);
    return STATUS_SUCCESS;
}

#pragma endregion

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
