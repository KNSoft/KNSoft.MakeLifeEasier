#include "NTAssassin.Lib.inl"

static SECURITY_QUALITY_OF_SERVICE g_DuplicateTokenSqos = {
    sizeof(g_DuplicateTokenSqos),
    SecurityImpersonation,
    0,
    FALSE
};

static OBJECT_ATTRIBUTES g_DuplicateTokenObjectAttribute = {
    sizeof(g_DuplicateTokenObjectAttribute),
    NULL,
    NULL,
    0,
    NULL,
    &g_DuplicateTokenSqos
};

HANDLE NTAPI NT_GetCurrentThreadToken()
{
    NTSTATUS Status;
    HANDLE hToken, hPrimaryToken;

    Status = NtOpenThreadToken(NtCurrentThread(), TOKEN_QUERY, FALSE, &hToken);
    if (NT_SUCCESS(Status))
    {
        return hToken;
    } else if (Status != STATUS_NO_TOKEN)
    {
        goto _error;
    }

    /* Duplicate the primary token to create an impersonation token */
    Status = NtOpenProcessToken(NtCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE, &hPrimaryToken);
    if (!NT_SUCCESS(Status))
    {
        goto _error;
    }

    Status = NtDuplicateToken(hPrimaryToken,
                              TOKEN_QUERY | TOKEN_IMPERSONATE,
                              &g_DuplicateTokenObjectAttribute,
                              FALSE,
                              TokenImpersonation,
                              &hToken);
    NtClose(hPrimaryToken);
    if (NT_SUCCESS(Status))
    {
        return hToken;
    }

_error:
    NtSetLastStatus(Status);
    return NULL;
}

static DECLSPEC_ALIGN(4) struct
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
} g_AdminGroupSD = {
    {
        SECURITY_DESCRIPTOR_REVISION,
        0,
        SE_DACL_PRESENT,
        &g_AdminGroupDacl.Ace.Sid2,
        &g_AdminGroupDacl.Ace.Sid2,
        NULL,
        &g_AdminGroupDacl.Acl
    }
};

static GENERIC_MAPPING g_GenericMapping = {
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    STANDARD_RIGHTS_ALL
};

NTSTATUS NTAPI NT_IsAdminToken(_In_opt_ HANDLE TokenHandle)
{
    ACCESS_MASK GrantedAccess;
    DEFINE_ANYSIZE_STRUCT(PrivilegeSet, PRIVILEGE_SET, LUID_AND_ATTRIBUTES, 4);
    ULONG PrivilegeSetSize = sizeof(PrivilegeSet);

    HANDLE hToken = NULL;
    NTSTATUS Status, AccessStatus;

    if (TokenHandle == NULL)
    {
        hToken = NT_GetCurrentThreadToken();
        if (hToken == NULL)
        {
            return NtGetLastStatus();
        }
    } else
    {
        hToken = TokenHandle;
    }

    Status = NtAccessCheck(&g_AdminGroupSD.Sd,
                           hToken,
                           g_AdminGroupDacl.Ace.AccessMask,
                           &g_GenericMapping,
                           &PrivilegeSet.BaseType,
                           &PrivilegeSetSize,
                           &GrantedAccess,
                           &AccessStatus);
    if (hToken != TokenHandle)
    {
        NtClose(hToken);
    }

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
