#include "Test.h"

typedef struct _TEST_CBS_CONTEXT
{
    PCWSTR TargetName;
    SYS_CBS_FEATURE_STATE TargetState;
    ULONG FeatureCount;
    ULONG ParentCount;
    LOGICAL FoundTarget;
} TEST_CBS_CONTEXT, *PTEST_CBS_CONTEXT;

static
_Function_class_(SYS_CBS_FEATURE_CALLBACK)
HRESULT
CALLBACK
Test_CbsFeatureCallback(
    _In_ PCSYS_CBS_FEATURE Feature,
    _In_opt_ PVOID Context)
{
    PTEST_CBS_CONTEXT TestContext;

    if (Context == NULL)
    {
        return E_UNEXPECTED;
    }
    TestContext = Context;
    TestContext->FeatureCount++;
    if (TestContext->TargetName != NULL &&
        CompareStringOrdinal(Feature->Name, -1, TestContext->TargetName, -1, TRUE) == CSTR_EQUAL)
    {
        TestContext->TargetState = Feature->State;
        TestContext->FoundTarget = TRUE;
    }
    return S_OK;
}

static
_Function_class_(SYS_CBS_FEATURE_PARENT_CALLBACK)
HRESULT
CALLBACK
Test_CbsParentCallback(
    _In_ PCSYS_CBS_FEATURE Feature,
    _In_ PCWSTR ParentName,
    _In_opt_ PCWSTR ParentSet,
    _In_opt_ PVOID Context)
{
    PTEST_CBS_CONTEXT TestContext;

    UNREFERENCED_PARAMETER(Feature);
    UNREFERENCED_PARAMETER(ParentName);
    UNREFERENCED_PARAMETER(ParentSet);
    if (Context == NULL)
    {
        return E_UNEXPECTED;
    }
    TestContext = Context;
    TestContext->ParentCount++;
    return S_OK;
}

TEST_FUNC(CBS_EnumerateFeatures)
{
    TEST_CBS_CONTEXT Context = { 0 };
    HRESULT Hr;

    Hr = Sys_CbsEnumerateFeatures(L"KNSoft.MakeLifeEasier.Test",
                                  Test_CbsFeatureCallback,
                                  Test_CbsParentCallback,
                                  &Context);
    if (FAILED(Hr))
    {
        TEST_SKIP("Sys_CbsEnumerateFeatures failed with 0x%08X\n", (UINT)Hr);
        return;
    }
    TEST_OK(Context.FeatureCount != 0);
    TEST_OK(Context.ParentCount != 0);
}

TEST_FUNC(CBS_ChangeFeature)
{
    TEST_CBS_CONTEXT Context = { L"TelnetClient" };
    CBS_REQUIRED_ACTION RequiredAction;
    LOGICAL Enable;
    HRESULT Hr, RestoreHr;

    Hr = Sys_CbsEnumerateFeatures(L"KNSoft.MakeLifeEasier.Test",
                                  Test_CbsFeatureCallback,
                                  Test_CbsParentCallback,
                                  &Context);
    if (FAILED(Hr) || !Context.FoundTarget ||
        (Context.TargetState.Current != CbsInstallStateAbsent &&
         Context.TargetState.Current != CbsInstallStateInstalled))
    {
        TEST_SKIP("TelnetClient is unavailable or not in a stable state\n");
        return;
    }
    Enable = Context.TargetState.Current == CbsInstallStateAbsent;
    Hr = Sys_CbsSetFeatureEnabled(L"KNSoft.MakeLifeEasier.Test",
                                  Context.TargetName,
                                  Enable,
                                  TRUE,
                                  &RequiredAction);
    TEST_OK(SUCCEEDED(Hr));
    if (FAILED(Hr))
    {
        return;
    }
    RestoreHr = Sys_CbsSetFeatureEnabled(L"KNSoft.MakeLifeEasier.Test",
                                         Context.TargetName,
                                         !Enable,
                                         TRUE,
                                         &RequiredAction);
    TEST_OK(SUCCEEDED(RestoreHr));
}
