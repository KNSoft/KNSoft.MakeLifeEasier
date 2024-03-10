#include "NTAssassin.Lib.inl"

#include <NTAssassin/Lib/NT.Security.h>

static CONST SECURITY_QUALITY_OF_SERVICE g_DuplicateTokenDefaultSqos = {
    sizeof(g_DuplicateTokenDefaultSqos),
    SecurityImpersonation,
    0,
    FALSE
};

static CONST OBJECT_ATTRIBUTES g_DuplicateTokenDefaultObjectAttribute = {
    sizeof(g_DuplicateTokenDefaultObjectAttribute),
    NULL,
    NULL,
    0,
    NULL,
    (PVOID)&g_DuplicateTokenDefaultSqos
};

HANDLE NTAPI NT_DuplicateToken(_In_ HANDLE ExistingToken, _In_ TOKEN_TYPE TokenType)
{
    NTSTATUS Status;
    HANDLE NewToken;

    Status = NtDuplicateToken(ExistingToken,
                              TOKEN_QUERY | TOKEN_IMPERSONATE,
                              (POBJECT_ATTRIBUTES)&g_DuplicateTokenDefaultObjectAttribute,
                              FALSE,
                              TokenImpersonation,
                              &NewToken);
    if (!NT_SUCCESS(Status))
    {
        NtSetLastStatus(Status);
        return NULL;
    }

    return NewToken;
}

HANDLE NTAPI NT_GetCurrentThreadImpersonationToken()
{
    NTSTATUS Status;
    HANDLE TokenHandle, PrimaryTokenHandle;

    /* Get current impersonation token if in impersonating */
    if (NtCurrentTeb()->IsImpersonating)
    {
        Status = NtOpenThreadToken(NtCurrentThread(), TOKEN_QUERY, FALSE, &TokenHandle);
        if (NT_SUCCESS(Status))
        {
            return TokenHandle;
        } else if (Status != STATUS_NO_TOKEN)
        {
            goto _error;
        }
    }

    /* Duplicate the primary token to create an impersonation token */
    Status = NtOpenProcessToken(NtCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE, &PrimaryTokenHandle);
    if (!NT_SUCCESS(Status))
    {
        goto _error;
    }
    Status = NtDuplicateToken(PrimaryTokenHandle,
                              TOKEN_QUERY | TOKEN_IMPERSONATE,
                              (POBJECT_ATTRIBUTES)&g_DuplicateTokenDefaultObjectAttribute,
                              FALSE,
                              TokenImpersonation,
                              &TokenHandle);
    NtClose(PrimaryTokenHandle);
    if (NT_SUCCESS(Status))
    {
        return TokenHandle;
    }

_error:
    NtSetLastStatus(Status);
    return NULL;
}

NTSTATUS NTAPI NT_IsCurrentAdminToken()
{
    NTSTATUS Status;
    HANDLE TokenHandle;

    TokenHandle = NT_GetCurrentThreadImpersonationToken();
    if (TokenHandle == NULL)
    {
        return NtGetLastStatus();
    }

    Status = NT_IsAdminToken(TokenHandle);
    NtClose(TokenHandle);
    return Status;
}

static CONST DECLSPEC_ALIGN(4) struct
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
        SID_ADMINS
    }
};

static DECLSPEC_ALIGN(SIZE_OF_POINTER) struct
{
    DECLSPEC_ALIGN(SIZE_OF_POINTER) SECURITY_DESCRIPTOR Sd;
} CONST g_AdminGroupSD = {
    {
        SECURITY_DESCRIPTOR_REVISION,
        0,
        SE_DACL_PRESENT,
        (PSID)&g_AdminGroupDacl.Ace.Sid2,
        (PSID)&g_AdminGroupDacl.Ace.Sid2,
        NULL,
        (PACL)&g_AdminGroupDacl.Acl
    }
};

static CONST GENERIC_MAPPING g_GenericMapping = {
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    STANDARD_RIGHTS_ALL
};

NTSTATUS NTAPI NT_IsAdminToken(_In_ HANDLE ImpersonationTokenHandle)
{
    NTSTATUS Status, AccessStatus;
    ACCESS_MASK GrantedAccess;
    DEFINE_ANYSIZE_STRUCT(PrivilegeSet, PRIVILEGE_SET, LUID_AND_ATTRIBUTES, 4);
    ULONG PrivilegeSetSize = sizeof(PrivilegeSet);

    Status = NtAccessCheck((PSECURITY_DESCRIPTOR)&g_AdminGroupSD.Sd,
                           ImpersonationTokenHandle,
                           g_AdminGroupDacl.Ace.AccessMask,
                           (PGENERIC_MAPPING)&g_GenericMapping,
                           &PrivilegeSet.BaseType,
                           &PrivilegeSetSize,
                           &GrantedAccess,
                           &AccessStatus);
    if (NT_SUCCESS(Status))
    {
        if (!NT_SUCCESS(AccessStatus))
        {
            return AccessStatus;
        } else if (GrantedAccess != g_AdminGroupDacl.Ace.AccessMask)
        {
            return STATUS_NOT_ALL_ASSIGNED;
        }
    }
    return Status;
}
