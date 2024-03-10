#include "../NT/MinDef.h"

EXTERN_C_START

EXTERN_C PSID SeWorldSid;
EXTERN_C PSID SeLocalSystemSid;
EXTERN_C PSID SeAliasAdminsSid;
EXTERN_C PSID SeAliasUsersSid;
EXTERN_C PSID SeAuthenticatedUsersSid;
EXTERN_C PSID SeLocalServiceSid;
EXTERN_C PSID SeNetworkServiceSid;
EXTERN_C PSID SeTrustedInstallerSid;
EXTERN_C PSID SeLocalSid;
EXTERN_C PSID SeInteractiveSid;
EXTERN_C PSID SeUntrustedMandatorySid;
EXTERN_C PSID SeLowMandatorySid;
EXTERN_C PSID SeMediumMandatorySid;
EXTERN_C PSID SeHighMandatorySid;
EXTERN_C PSID SeSystemMandatorySid;

EXTERN_C PSID SeLocalAccountSid;
EXTERN_C PSID SeLocalAccountAndAdminSid;
EXTERN_C PSID SeLocalLogonSid;
EXTERN_C PSID SeNTLMSid;
EXTERN_C PSID SeThisOrganizationSid;
EXTERN_C PSID SeMediumPlusMandatorySid;
EXTERN_C PSID SeProtectedProcessMandatorySid;

HANDLE NTAPI NT_DuplicateToken(_In_ HANDLE ExistingToken, _In_ TOKEN_TYPE TokenType);

HANDLE NTAPI NT_GetCurrentThreadImpersonationToken();

NTSTATUS NTAPI NT_IsCurrentAdminToken();

NTSTATUS NTAPI NT_IsAdminToken(_In_ HANDLE ImpersonationTokenHandle);

EXTERN_C_END
