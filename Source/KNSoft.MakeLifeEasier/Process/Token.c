#include "../MakeLifeEasier.inl"

static CONST SECURITY_QUALITY_OF_SERVICE g_ImpersonationSqos = {
    sizeof(g_ImpersonationSqos),
    SecurityImpersonation,
    0,
    FALSE
};

static CONST OBJECT_ATTRIBUTES g_ImpersonationObjectAttribute = {
    sizeof(g_ImpersonationObjectAttribute),
    NULL,
    NULL,
    0,
    NULL,
    (PVOID)&g_ImpersonationSqos
};

NTSTATUS
NTAPI
PS_DuplicateToken(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_TYPE TokenType,
    _Out_ PHANDLE NewTokenHandle)
{
    return NtDuplicateToken(TokenHandle,
                            TOKEN_QUERY | TOKEN_IMPERSONATE,
                            (POBJECT_ATTRIBUTES)(&g_ImpersonationObjectAttribute),
                            FALSE,
                            TokenType,
                            NewTokenHandle);
}

NTSTATUS
NTAPI
PS_OpenCurrentThreadToken(
    _Out_ PHANDLE TokenHandle)
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

    Status = NtOpenProcessToken(NtCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE, &hPrimaryToken);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    Status = PS_DuplicateToken(hPrimaryToken, TokenImpersonation, TokenHandle);
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

static CONST DECLSPEC_ALIGN(SIZE_OF_POINTER) SECURITY_DESCRIPTOR g_AdminGroupSD = {
    SECURITY_DESCRIPTOR_REVISION,
    0,
    SE_DACL_PRESENT,
    (PSID)&g_AdminGroupDacl.Ace.Sid2.BaseType,
    (PSID)&g_AdminGroupDacl.Ace.Sid2.BaseType,
    NULL,
    (PACL)&g_AdminGroupDacl.Acl
};

static CONST GENERIC_MAPPING g_GenericMapping = {
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
PS_IsCurrentAdminToken(VOID)
{
    NTSTATUS Status;
    HANDLE Token;

    Status = PS_OpenCurrentThreadToken(&Token);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = PS_IsAdminToken(Token);
    NtClose(Token);
    return Status;
}
