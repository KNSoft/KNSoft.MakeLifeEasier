#include "NTAssassin.Lib.inl"

static CONST SECURITY_QUALITY_OF_SERVICE g_DuplicateTokenSqos = {
    sizeof(g_DuplicateTokenSqos),
    SecurityImpersonation,
    0,
    FALSE
};

static CONST OBJECT_ATTRIBUTES g_DuplicateTokenObjectAttribute = {
    sizeof(g_DuplicateTokenObjectAttribute),
    NULL,
    NULL,
    0,
    NULL,
    RTL_CONST_CAST(PVOID)(&g_DuplicateTokenSqos)
};

NTSTATUS NTAPI NT_DuplicateToken(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_TYPE TokenType,
    _Out_ PHANDLE NewTokenHandle)
{
    return NtDuplicateToken(TokenHandle,
                            TOKEN_QUERY | TOKEN_IMPERSONATE,
                            RTL_CONST_CAST(POBJECT_ATTRIBUTES)(&g_DuplicateTokenObjectAttribute),
                            FALSE,
                            TokenType,
                            NewTokenHandle);
}

NTSTATUS NTAPI NT_GetCurrentThreadToken(_Out_ PHANDLE TokenHandle)
{
    NTSTATUS Status;
    HANDLE hPrimaryToken;

    if (NtCurrentTeb()->IsImpersonating)
    {
        Status = NtOpenThreadToken(NtCurrentThread(), TOKEN_QUERY, FALSE, TokenHandle);
        if (Status != STATUS_NO_TOKEN)
        {
            return Status;
        }
    }

    /* Duplicate the primary token to create an impersonation token */
    Status = NtOpenProcessToken(NtCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE, &hPrimaryToken);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    Status = NT_DuplicateToken(hPrimaryToken, TokenImpersonation, TokenHandle);
    NtClose(hPrimaryToken);
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

static CONST DECLSPEC_ALIGN(SIZE_OF_POINTER) struct
{
    DECLSPEC_ALIGN(SIZE_OF_POINTER) SECURITY_DESCRIPTOR Sd;
} g_AdminGroupSD = {
    {
        SECURITY_DESCRIPTOR_REVISION,
        0,
        SE_DACL_PRESENT,
        RTL_CONST_CAST(PSID)(&g_AdminGroupDacl.Ace.Sid2),
        RTL_CONST_CAST(PSID)(&g_AdminGroupDacl.Ace.Sid2),
        NULL,
        RTL_CONST_CAST(PACL)(&g_AdminGroupDacl.Acl)
    }
};

static CONST GENERIC_MAPPING g_GenericMapping = {
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    STANDARD_RIGHTS_ALL
};

NTSTATUS NTAPI NT_IsAdminToken(_In_ HANDLE TokenHandle)
{
    NTSTATUS Status, AccessStatus;
    ACCESS_MASK GrantedAccess;
    DEFINE_ANYSIZE_STRUCT(PrivilegeSet, PRIVILEGE_SET, LUID_AND_ATTRIBUTES, 4);
    ULONG PrivilegeSetSize = sizeof(PrivilegeSet);

    Status = NtAccessCheck(RTL_CONST_CAST(PSECURITY_DESCRIPTOR)(&g_AdminGroupSD.Sd),
                           TokenHandle,
                           g_AdminGroupDacl.Ace.AccessMask,
                           RTL_CONST_CAST(PGENERIC_MAPPING)(&g_GenericMapping),
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

NTSTATUS NTAPI NT_IsCurrentAdminToken()
{
    NTSTATUS Status;
    HANDLE hToken;

    Status = NT_GetCurrentThreadToken(&hToken);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = NT_IsAdminToken(hToken);
    NtClose(hToken);
    return Status;
}
