#include "../MakeLifeEasier.inl"

NTSTATUS
NTAPI
PS_DuplicateToken(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_TYPE TokenType,
    _Out_ PHANDLE NewTokenHandle)
{
    return NtDuplicateToken(TokenHandle,
                            MAXIMUM_ALLOWED,
                            (POBJECT_ATTRIBUTES)(&NT_DefaultImpersonationObjectAttribute),
                            FALSE,
                            TokenType,
                            NewTokenHandle);
}

NTSTATUS
NTAPI
PS_DuplicateSystemToken(
    _In_ ULONG ProcessId,
    _In_ TOKEN_TYPE TokenType,
    _Out_ PHANDLE NewTokenHandle)
{
    NTSTATUS Status;
    HANDLE ProcessHandle, TokenHandle;
    CLIENT_ID ClientId;

    ClientId.UniqueProcess = (HANDLE)(ULONG_PTR)ProcessId;
    ClientId.UniqueThread = 0;

    Status = NtOpenProcess(&ProcessHandle, PROCESS_QUERY_LIMITED_INFORMATION, &NT_EmptyObjectAttribute, &ClientId);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    Status = NtOpenProcessToken(ProcessHandle, TOKEN_QUERY | TOKEN_DUPLICATE, &TokenHandle);
    if (!NT_SUCCESS(Status))
    {
        goto _Exit;
    }
    Status = NtDuplicateToken(TokenHandle,
                              MAXIMUM_ALLOWED,
                              TokenType == TokenImpersonation ? (POBJECT_ATTRIBUTES)&NT_DefaultImpersonationObjectAttribute : NULL,
                              FALSE,
                              TokenType,
                              NewTokenHandle);
    NtClose(TokenHandle);

_Exit:
    NtClose(ProcessHandle);
    return Status;
}

NTSTATUS
NTAPI
PS_OpenCurrentThreadToken(
    _Out_ PHANDLE TokenHandle)
{
    NTSTATUS Status;
    HANDLE PrimaryToken;

    if (NtCurrentTeb()->IsImpersonating)
    {
        Status = NtOpenThreadToken(NtCurrentThread(), TOKEN_QUERY, FALSE, TokenHandle);
        if (Status != STATUS_NO_TOKEN)
        {
            return Status;
        }
    }

    Status = NtOpenProcessToken(NtCurrentProcess(), MAXIMUM_ALLOWED, &PrimaryToken);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    Status = PS_DuplicateToken(PrimaryToken, TokenImpersonation, TokenHandle);
    NtClose(PrimaryToken);
    return Status;
}

static const DECLSPEC_ALIGN(4) struct
{
    DECLSPEC_ALIGN(4) ACL Acl;
    DECLSPEC_ALIGN(4) struct
    {
        ACE_HEADER Header;
        ACCESS_MASK AccessMask;
        SID_2 Sid2;
    } Ace; /* ACCESS_ALLOWED_ACE */
} g_AdminGroupDacl = {
    {
        MIN_ACL_REVISION,
        0,
        sizeof(g_AdminGroupDacl),
        1,
        0
    },
    {
        { 0, 0, sizeof(g_AdminGroupDacl.Ace) },
        1,
        SID_BUILTIN_ADMINISTRATORS
    }
};

static const DECLSPEC_POINTERALIGN SECURITY_DESCRIPTOR g_AdminGroupSD = {
    SECURITY_DESCRIPTOR_REVISION,
    0,
    SE_DACL_PRESENT,
    (PSID)&g_AdminGroupDacl.Ace.Sid2.BaseType,
    (PSID)&g_AdminGroupDacl.Ace.Sid2.BaseType,
    NULL,
    (PACL)&g_AdminGroupDacl.Acl
};

static const GENERIC_MAPPING g_GenericMapping = {
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    STANDARD_RIGHTS_ALL
};

NTSTATUS
NTAPI
PS_IsAdminToken(
    _In_ HANDLE TokenHandle)
{
    NTSTATUS Status, AccessStatus;
    ACCESS_MASK GrantedAccess;
    DEFINE_ANYSIZE_STRUCT(PrivilegeSet, PRIVILEGE_SET, LUID_AND_ATTRIBUTES, 4);
    ULONG PrivilegeSetSize = sizeof(PrivilegeSet);

    Status = NtAccessCheck((PSECURITY_DESCRIPTOR)(&g_AdminGroupSD),
                           TokenHandle,
                           g_AdminGroupDacl.Ace.AccessMask,
                           (PGENERIC_MAPPING)(&g_GenericMapping),
                           &PrivilegeSet.BaseType,
                           &PrivilegeSetSize,
                           &GrantedAccess,
                           &AccessStatus);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    } else if (AccessStatus != STATUS_SUCCESS)
    {
        return AccessStatus;
    }

    return GrantedAccess == g_AdminGroupDacl.Ace.AccessMask ? STATUS_SUCCESS : STATUS_NOT_ALL_ASSIGNED;
}

NTSTATUS
NTAPI
PS_GetTokenInfo(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
    _Out_ PVOID* Info)
{
    PVOID Buffer;
    ULONG Length;
    NTSTATUS Status;

    Status = NtQueryInformationToken(TokenHandle, TokenInformationClass, NULL, 0, &Length);
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        return !NT_SUCCESS(Status) ? Status : STATUS_INVALID_INFO_CLASS;
    }
    Buffer = Mem_Alloc(Length);
    if (Buffer == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    Status = NtQueryInformationToken(TokenHandle, TokenInformationClass, Buffer, Length, &Length);
    if (NT_SUCCESS(Status))
    {
        *Info = Buffer;
    } else
    {
        Mem_Free(Buffer);
    }
    return Status;
}
