#include "Test.h"

typedef struct _TEST_FVE_CONTEXT
{
    ULONG VolumeCount;
    ULONG StatusCount;
} TEST_FVE_CONTEXT, *PTEST_FVE_CONTEXT;

static
_Function_class_(SYS_FVE_VOLUME_CALLBACK)
HRESULT
CALLBACK
Test_FveVolumeCallback(
    _In_ PCWSTR VolumeName,
    _In_ FVE_DEVICE_TYPE DeviceType,
    _In_opt_ PVOID Context)
{
    PTEST_FVE_CONTEXT TestContext;
    FVE_STATUS_V9 Status;
    HANDLE VolumeHandle;
    HRESULT Hr;

    if (Context == NULL ||
        (DeviceType != FVE_DEVICE_VOLUME && DeviceType != FVE_DEVICE_CSV_VOLUME))
    {
        return E_UNEXPECTED;
    }
    TestContext = Context;
    TestContext->VolumeCount++;
    Hr = FveOpenVolumeW(VolumeName, FALSE, &VolumeHandle);
    if (FAILED(Hr))
    {
        return Hr;
    }
    Hr = Sys_FveGetStatus(VolumeHandle, &Status);
    if (SUCCEEDED(Hr))
    {
        TestContext->StatusCount++;
    }
    FveCloseVolume(VolumeHandle);
    return Hr;
}

TEST_FUNC(FVE_EnumerateVolumes)
{
    TEST_FVE_CONTEXT Context = { 0 };
    HRESULT Hr;

    Hr = Sys_FveEnumerateVolumes(Test_FveVolumeCallback, &Context);
    if (FAILED(Hr))
    {
        TEST_SKIP("Sys_FveEnumerateVolumes failed with 0x%08X\n", (UINT)Hr);
        return;
    }
    TEST_OK(Context.VolumeCount != 0);
    TEST_OK(Context.StatusCount == Context.VolumeCount);
}

static
HRESULT
Test_FveWaitForStatus(
    _In_ HANDLE VolumeHandle,
    _In_ ULONG SetFlags,
    _In_ ULONG ClearFlags)
{
    FVE_STATUS_V9 Status;
    ULONG Index;
    HRESULT Hr;

    for (Index = 0; Index < 1200; Index++)
    {
        Hr = Sys_FveGetStatus(VolumeHandle, &Status);
        if (FAILED(Hr) ||
            ((Status.Flags & SetFlags) == SetFlags && (Status.Flags & ClearFlags) == 0))
        {
            return Hr;
        }
        Sleep(100);
    }
    return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
}

static
HRESULT
Test_FveDecryptWhenReady(
    _In_ HANDLE VolumeHandle)
{
    ULONG Index;
    HRESULT Hr;

    for (Index = 0; Index < 100; Index++)
    {
        Hr = Sys_FveDecrypt(VolumeHandle);
        if (Hr != HRESULT_FROM_WIN32(ERROR_NOT_READY))
        {
            return Hr;
        }
        Sleep(100);
    }
    return Hr;
}

TEST_FUNC(FVE_VolumeLifecycle)
{
    PFVE_AUTH_INFORMATION Information;
    FVE_STATUS_V9 Status;
    PGUID AuthMethodGuids;
    GUID AuthMethodGuid;
    HANDLE VolumeHandle;
    SIZE_T InformationSize;
    UINT AuthMethodCount;
    HRESULT Hr;

    if (TEST_PARAMETER_ARGC != 2)
    {
        TEST_SKIP("Usage: FVE_VolumeLifecycle <volume-name> <recovery-password>\n");
        return;
    }
    Hr = FveOpenVolumeW(TEST_PARAMETER_ARGV[0], TRUE, &VolumeHandle);
    TEST_OK(SUCCEEDED(Hr));
    if (FAILED(Hr))
    {
        return;
    }
    Hr = Sys_FveGetStatus(VolumeHandle, &Status);
    TEST_OK(SUCCEEDED(Hr));
    if (FAILED(Hr))
    {
        goto _Close_Volume;
    }
    if ((Status.Flags & FVE_STATUS_FLAG_INITIALIZED) != 0)
    {
        Hr = Test_FveDecryptWhenReady(VolumeHandle);
        TEST_OK(SUCCEEDED(Hr));
        if (FAILED(Hr))
        {
            goto _Close_Volume;
        }
        Hr = Sys_FveGetStatus(VolumeHandle, &Status);
        if (SUCCEEDED(Hr) && (Status.Flags & FVE_STATUS_FLAG_INITIALIZED) != 0)
        {
            if ((Status.Flags & FVE_STATUS_FLAG_FULLY_DECRYPTED) == 0)
            {
                Hr = Test_FveWaitForStatus(VolumeHandle,
                                           FVE_STATUS_FLAG_FULLY_DECRYPTED,
                                           FVE_STATUS_FLAG_DECRYPTION_IN_PROGRESS);
            }
            if (SUCCEEDED(Hr))
            {
                Hr = Test_FveDecryptWhenReady(VolumeHandle);
            }
        }
        TEST_OK(SUCCEEDED(Hr));
        if (FAILED(Hr))
        {
            goto _Close_Volume;
        }
    }
    Hr = Sys_FveGetStatus(VolumeHandle, &Status);
    TEST_OK(SUCCEEDED(Hr));
    if (FAILED(Hr))
    {
        goto _Close_Volume;
    }
    Hr = Sys_FveGetAuthMethodGuids(VolumeHandle, &AuthMethodGuids, &AuthMethodCount);
    if (Hr == FVE_E_NOT_ACTIVATED)
    {
        Hr = S_OK;
        AuthMethodGuids = NULL;
        AuthMethodCount = 0;
    }
    TEST_OK(SUCCEEDED(Hr));
    if (FAILED(Hr))
    {
        goto _Close_Volume;
    }
    if (AuthMethodCount != 0)
    {
        Mem_Free(AuthMethodGuids);
        TEST_SKIP("The test volume already has a key protector (count %u, flags 0x%08X)\n",
                  AuthMethodCount,
                  Status.Flags);
        goto _Close_Volume;
    }
    Mem_Free(AuthMethodGuids);
    Hr = Sys_FveAddRecoveryPasswordProtector(VolumeHandle,
                                              TEST_PARAMETER_ARGV[1],
                                              L"KNSoft.MakeLifeEasier test",
                                              &AuthMethodGuid);
    TEST_OK(SUCCEEDED(Hr));
    if (FAILED(Hr))
    {
        goto _Close_Volume;
    }
    Hr = Sys_FveGetAuthMethodGuids(VolumeHandle, &AuthMethodGuids, &AuthMethodCount);
    TEST_OK(SUCCEEDED(Hr));
    if (FAILED(Hr))
    {
        goto _Restore_Volume;
    }
    TEST_OK(AuthMethodCount == 1 &&
            AuthMethodGuids != NULL &&
            RtlEqualMemory(AuthMethodGuids, &AuthMethodGuid, sizeof(AuthMethodGuid)));
    Mem_Free(AuthMethodGuids);
    Hr = Sys_FveGetAuthMethodInformation(VolumeHandle,
                                         &AuthMethodGuid,
                                         FVE_AUTH_INFORMATION_QUERY_UNKNOWN1,
                                         &Information,
                                         &InformationSize);
    TEST_OK(SUCCEEDED(Hr));
    if (FAILED(Hr))
    {
        goto _Restore_Volume;
    }
    TEST_OK((Information->AuthFlags & FVE_AUTH_INFORMATION_PROTECTOR_MASK) ==
            FVE_AUTH_INFORMATION_FLAG_RECOVERY_PASSWORD);
    Sys_FveFreeAuthMethodInformation(Information, InformationSize);
    Hr = Sys_FveEncrypt(VolumeHandle, FveLegacyMethodXtsAes128, TRUE);
    TEST_OK(SUCCEEDED(Hr));
    if (FAILED(Hr))
    {
        goto _Restore_Volume;
    }
    Hr = Test_FveWaitForStatus(VolumeHandle,
                               FVE_STATUS_FLAG_FULLY_ENCRYPTED,
                               FVE_STATUS_FLAG_ENCRYPTION_IN_PROGRESS);
    TEST_OK(SUCCEEDED(Hr));
    if (FAILED(Hr))
    {
        goto _Restore_Volume;
    }
    Hr = Sys_FveDeleteProtector(VolumeHandle, &AuthMethodGuid);
    TEST_OK(Hr == FVE_E_KEY_REQUIRED);
    if (Hr != FVE_E_KEY_REQUIRED)
    {
        goto _Restore_Volume;
    }
    Hr = Sys_FveDisableProtectors(VolumeHandle, SYS_FVE_DISABLE_COUNT_DEFAULT);
    TEST_OK(SUCCEEDED(Hr));
    if (FAILED(Hr))
    {
        goto _Restore_Volume;
    }
    Hr = Sys_FveGetStatus(VolumeHandle, &Status);
    TEST_OK(SUCCEEDED(Hr) && (Status.Flags & FVE_STATUS_FLAG_CLEAR_KEY) != 0);
    if (FAILED(Hr) || (Status.Flags & FVE_STATUS_FLAG_CLEAR_KEY) == 0)
    {
        goto _Restore_Volume;
    }
    Hr = Sys_FveEnableProtectors(VolumeHandle);
    TEST_OK(SUCCEEDED(Hr));
    if (FAILED(Hr))
    {
        goto _Restore_Volume;
    }
    Hr = Sys_FveGetStatus(VolumeHandle, &Status);
    TEST_OK(SUCCEEDED(Hr) && (Status.Flags & FVE_STATUS_FLAG_CLEAR_KEY) == 0);
    if (FAILED(Hr) || (Status.Flags & FVE_STATUS_FLAG_CLEAR_KEY) != 0)
    {
        goto _Restore_Volume;
    }
    Hr = FveLockVolume(VolumeHandle, FALSE);
    TEST_OK(SUCCEEDED(Hr));
    if (SUCCEEDED(Hr))
    {
        Hr = Sys_FveUnlockWithRecoveryPassword(VolumeHandle, TEST_PARAMETER_ARGV[1]);
        TEST_OK(SUCCEEDED(Hr));
        if (SUCCEEDED(Hr))
        {
            FveCloseVolume(VolumeHandle);
            Hr = FveOpenVolumeW(TEST_PARAMETER_ARGV[0], TRUE, &VolumeHandle);
            TEST_OK(SUCCEEDED(Hr));
            if (FAILED(Hr))
            {
                return;
            }
        }
    }
    if (FAILED(Hr))
    {
        goto _Restore_Volume;
    }
    Hr = Test_FveDecryptWhenReady(VolumeHandle);
    TEST_OK(SUCCEEDED(Hr));
    if (FAILED(Hr))
    {
        goto _Restore_Volume;
    }
    Hr = Test_FveWaitForStatus(VolumeHandle,
                               FVE_STATUS_FLAG_FULLY_DECRYPTED,
                               FVE_STATUS_FLAG_DECRYPTION_IN_PROGRESS);
    TEST_OK(SUCCEEDED(Hr));
    if (FAILED(Hr))
    {
        goto _Restore_Volume;
    }
    Hr = Sys_FveDeleteProtector(VolumeHandle, &AuthMethodGuid);
    TEST_OK(SUCCEEDED(Hr) || Hr == FVE_E_NOT_ACTIVATED);
    if (FAILED(Hr) && Hr != FVE_E_NOT_ACTIVATED)
    {
        goto _Restore_Volume;
    }
    goto _Close_Volume;

_Restore_Volume:
    Sys_FveUnlockWithRecoveryPassword(VolumeHandle, TEST_PARAMETER_ARGV[1]);
    Test_FveDecryptWhenReady(VolumeHandle);
    Test_FveWaitForStatus(VolumeHandle,
                          FVE_STATUS_FLAG_FULLY_DECRYPTED,
                          FVE_STATUS_FLAG_DECRYPTION_IN_PROGRESS);
    FveSetFipsAllowDisabled(TRUE);
    FveRevertVolume(VolumeHandle);

_Close_Volume:
    FveCloseVolume(VolumeHandle);
}
