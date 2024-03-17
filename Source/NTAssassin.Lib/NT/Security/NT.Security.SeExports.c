#include "NTAssassin.Lib.inl"

/* SeExport */
PSID SeWorldSid = &(SID)SID_EVERYONE;
PSID SeLocalSystemSid = &(SID)SID_SYSTEM;
PSID SeAliasAdminsSid = &((SID_2)SID_ADMINS).BaseType;
PSID SeAliasUsersSid = &((SID_2)SID_USERS).BaseType;
PSID SeAuthenticatedUsersSid = &(SID)SID_AUTHENTICATED_USERS;
PSID SeLocalServiceSid = &(SID)SID_LOCAL_SERVICE;
PSID SeNetworkServiceSid = &(SID)SID_NETWORK_SERVICE;
PSID SeTrustedInstallerSid = &((SID_6)SID_TRUSTED_INSTALLER).BaseType;
PSID SeLocalSid = &(SID)SID_LOCAL;
PSID SeInteractiveSid = &(SID)SID_INTERACTIVE;
PSID SeUntrustedMandatorySid = &(SID)SID_MANDATORY_UNTRUSTED;
PSID SeLowMandatorySid = &(SID)SID_MANDATORY_LOW;
PSID SeMediumMandatorySid = &(SID)SID_MANDATORY_MEDIUM;
PSID SeHighMandatorySid = &(SID)SID_MANDATORY_HIGH;
PSID SeSystemMandatorySid = &(SID)SID_MANDATORY_SYSTEM;

/* Addendum */
PSID SeLocalAccountSid = &(SID)SID_LOCAL_ACCOUNT;
PSID SeLocalAccountAndAdminSid = &(SID)SID_LOCAL_ACCOUNT_AND_ADMIN;
PSID SeLocalLogonSid = &(SID)SID_LOCAL_LOGON;
PSID SeNTLMSid = &((SID_2)SID_NTLM).BaseType;
PSID SeThisOrganizationSid = &(SID)SID_THIS_ORGANIZATION;
PSID SeMediumPlusMandatorySid = &(SID)SID_MANDATORY_MEDIUM_PLUS;
PSID SeProtectedProcessMandatorySid = &(SID)SID_MANDATORY_PROTECTED_PROCESS;
