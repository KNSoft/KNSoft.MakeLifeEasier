/*
 * CreateSuperToken: Create a super token by calling NtCreateToken and run cmd.exe
 *   Super token:
 *     - All privileges (SE_*_PRIVILEGE) are enabled by default
 *     - With SYSTEM, Administrators, TrustedInstaller, ... permissions, but owned by Guests
 *     - Has highest integrity (SYSTEM)
 *     - Has UIAccess
 *
 * Run: "Test.exe -Run CreateSuperToken", need administrator privilege.
 */

#include "../Test.h"

TEST_FUNC(CreateSuperToken)
{
    NTSTATUS Status;

    /*** Impersonate LSA to obtain SE_CREATE_TOKEN_PRIVILEGE privilege ***/

    /* Get LSA process ID */
    ULONG LsaProcessId;
    Status = Sys_GetLsaProcessId(&LsaProcessId);
    if (!NT_SUCCESS(Status))
    {
        UnitTest_PrintF("L%-3lu: Sys_GetLsaProcessId failed with: 0x%08lX\n", __LINE__, Status);
        return;
    }

    /* Duplicate LSA impersonation token */
    HANDLE LsaImpersonateToken;
    Status = PS_DuplicateSystemToken(LsaProcessId, TokenImpersonation, &LsaImpersonateToken);
    if (!NT_SUCCESS(Status))
    {
        UnitTest_PrintF("L%-3lu: PS_DuplicateSystemToken failed with: 0x%08lX\n", __LINE__, Status);
        return;
    }

    /* Impersonate LSA */
    Status = PS_Impersonate(LsaImpersonateToken);
    if (!NT_SUCCESS(Status))
    {
        UnitTest_PrintF("L%-3lu: PS_Impersonate failed with: 0x%08lX\n", __LINE__, Status);
        goto _Exit_0;
    }

    /*** Create super token ***/

    /* Get current logon SID */
    PTOKEN_GROUPS LogonSidGroups;
    ULONG LogonSidLength;
    PSID LogonSid = NULL;
    Status = PS_GetTokenInfo(NtCurrentProcessToken(), TokenLogonSid, &LogonSidGroups);
    if (!NT_SUCCESS(Status))
    {
        UnitTest_PrintF("L%-3lu: PS_GetTokenInfo failed with: 0x%08lX\n", __LINE__, Status);
        goto _Exit_1;
    }
    for (ULONG i = 0; i < LogonSidGroups->GroupCount; i++)
    {
        if (LogonSidGroups->Groups[i].Attributes & SE_GROUP_LOGON_ID)
        {
            LogonSidLength = RtlLengthSid(LogonSidGroups->Groups[i].Sid);
            LogonSid = Mem_Alloc(LogonSidLength);
            if (LogonSid != NULL)
            {
                memcpy(LogonSid, LogonSidGroups->Groups[i].Sid, LogonSidLength);
            }
            break;
        }
    }
    Mem_Free(LogonSidGroups);
    if (LogonSid == NULL)
    {
        UnitTest_PrintF("L%-3lu: Current logon ID SID not found\n", __LINE__);
        goto _Exit_1;
    }

    /* Build token groups and privileges */
    SID EveryoneSid = SID_EVERYONE, SystemSid = SID_LOCAL_SYSTEM, AuthUsersSid = SID_AUTHENTICATED_USERS;
    SID_2 AdminsSid = SID_BUILTIN_ADMINISTRATORS, MandatorySid = SID_ML_SYSTEM, OwnerSid = SID_BUILTIN_GUESTS;
    SID_6 TISid = SID_TRUSTED_INSTALLER;
    LUID SystemLuid = SYSTEM_LUID;
#define GROUP_COUNT 7
    DEFINE_ANYSIZE_STRUCT(Groups, TOKEN_GROUPS, SID_AND_ATTRIBUTES, GROUP_COUNT) = {
        GROUP_COUNT,
        { &AdminsSid, SE_GROUP_MANDATORY | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_ENABLED },
        {
            { &SystemSid, SE_GROUP_MANDATORY | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_ENABLED },
            { &AuthUsersSid, SE_GROUP_MANDATORY | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_ENABLED },
            { &EveryoneSid, SE_GROUP_MANDATORY | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_ENABLED },
            { &TISid, SE_GROUP_MANDATORY | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_ENABLED },
            { &MandatorySid, SE_GROUP_INTEGRITY | SE_GROUP_INTEGRITY_ENABLED },
            { LogonSid, SE_GROUP_LOGON_ID | SE_GROUP_MANDATORY | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_ENABLED },
        }
    };
#undef GROUP_COUNT
#define PRIVILEGE_COUNT (SE_MAX_WELL_KNOWN_PRIVILEGE - SE_MIN_WELL_KNOWN_PRIVILEGE + 1)
    DEFINE_ANYSIZE_STRUCT(Privileges, TOKEN_PRIVILEGES, LUID_AND_ATTRIBUTES, PRIVILEGE_COUNT);
    PLUID_AND_ATTRIBUTES LuidAndAttr = Privileges.BaseType.Privileges;
    Privileges.BaseType.PrivilegeCount = PRIVILEGE_COUNT;
    for (LONG i = 0; i < PRIVILEGE_COUNT; i++)
    {
        Privileges.BaseType.Privileges[i].Luid = RtlConvertLongToLuid(i + SE_MIN_WELL_KNOWN_PRIVILEGE);
        Privileges.BaseType.Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT;
    }
#undef PRIVILEGE_COUNT

    /* Create token and set information */
    HANDLE SuperToken;
    Status = NT_CreateToken(&SuperToken,
                            TokenPrimary,
                            &OwnerSid.BaseType,
                            &SystemLuid,
                            &Groups.BaseType,
                            &Privileges.BaseType);
    if (!NT_SUCCESS(Status))
    {
        UnitTest_PrintF("L%-3lu: NT_CreateToken failed with: 0x%08lX\n", __LINE__, Status);
        goto _Exit_2;
    }
    ULONG UIAccess = TRUE;
    Status = NtSetInformationToken(SuperToken,
                                   TokenSessionId,
                                   &NtCurrentPeb()->SessionId,
                                   sizeof(NtCurrentPeb()->SessionId));
    if (!NT_SUCCESS(Status))
    {
        UnitTest_PrintF("L%-3lu: NtSetInformationToken failed with: 0x%08lX\n", __LINE__, Status);
    }
    Status = NtSetInformationToken(SuperToken, TokenUIAccess, &UIAccess, sizeof(UIAccess));
    if (!NT_SUCCESS(Status))
    {
        UnitTest_PrintF("L%-3lu: NtSetInformationToken failed with: 0x%08lX\n", __LINE__, Status);
    }

    /*** Run cmd.exe ***/

    WCHAR CmdPath[MAX_PATH];
    W32ERROR Ret;
    PROCESS_INFORMATION ProcessInfo;

    ULONG i = Str_CopyW(CmdPath, SharedUserData->NtSystemRoot);
    i = Str_CopyExW(CmdPath + i, ARRAYSIZE(CmdPath) - i, L"\\System32\\cmd.exe");
    Ret = PS_CreateProcess(SuperToken,
                           CmdPath,
                           CmdPath,
                           FALSE,
                           NULL,
                           NULL,
                           &ProcessInfo);
    if (Ret != ERROR_SUCCESS)
    {
        UnitTest_PrintF("L%-3lu: PS_CreateProcess failed with: 0x%08lX\n", __LINE__, Ret);
        goto _Exit_3;
    }
    NtClose(ProcessInfo.hThread);
    NtClose(ProcessInfo.hProcess);

_Exit_3:
    NtClose(SuperToken);
_Exit_2:
    Mem_Free(LogonSid);
_Exit_1:
    PS_Impersonate(NULL);
_Exit_0:
    NtClose(LsaImpersonateToken);
    return;
}
