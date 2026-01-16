#include "../MakeLifeEasier.inl"

CONST SECURITY_QUALITY_OF_SERVICE NT_DefaultImpersonationSqos = {
    sizeof(NT_DefaultImpersonationSqos),
    DEFAULT_IMPERSONATION_LEVEL,
    SECURITY_DYNAMIC_TRACKING,
    FALSE
};

CONST OBJECT_ATTRIBUTES NT_DefaultImpersonationObjectAttribute = {
    sizeof(NT_DefaultImpersonationObjectAttribute),
    NULL,
    NULL,
    0,
    NULL,
    (PVOID)&NT_DefaultImpersonationSqos
};

NTSTATUS
NTAPI
NT_CreateToken(
    _Out_ PHANDLE TokenHandle,
    _In_ TOKEN_TYPE Type,
    _In_ PSID OwnerSid,
    _In_ PLUID AuthenticationId,
    _In_ PTOKEN_GROUPS Groups,
    _In_ PTOKEN_PRIVILEGES Privileges)
{
    LARGE_INTEGER ExpirationTime = { 0 };
    TOKEN_OWNER Owner = { OwnerSid };
    TOKEN_PRIMARY_GROUP PrimaryGroup = { OwnerSid };
    TOKEN_USER User = { { OwnerSid } };
    TOKEN_SOURCE Source = { 0 };

    return NtCreateToken(TokenHandle,
                         TOKEN_ALL_ACCESS,
                         Type != TokenImpersonation ? NULL : (POBJECT_ATTRIBUTES)&NT_DefaultImpersonationObjectAttribute,
                         Type,
                         AuthenticationId,
                         &ExpirationTime,
                         &User,
                         Groups,
                         Privileges,
                         &Owner,
                         &PrimaryGroup,
                         NULL,
                         &Source);
}
