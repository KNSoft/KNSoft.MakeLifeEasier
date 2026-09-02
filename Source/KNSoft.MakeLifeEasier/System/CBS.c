#include "../MakeLifeEasier.inl"

#pragma comment(lib, "Ole32.lib")

#define SYS_CBS_FOUNDATION_PACKAGE L"@Foundation"

typedef struct _SYS_CBS_NAME_SET
{
    PWSTR* Names;
    ULONG Count;
    ULONG Capacity;
} SYS_CBS_NAME_SET, *PSYS_CBS_NAME_SET;

static
VOID
NTAPI
Sys_CbsFreeNameSet(
    _Inout_ PSYS_CBS_NAME_SET Set)
{
    ULONG Index;

    for (Index = 0; Index < Set->Count; Index++)
    {
        Mem_Free(Set->Names[Index]);
    }
    Mem_Free(Set->Names);
}

static
HRESULT
NTAPI
Sys_CbsAddName(
    _Inout_ PSYS_CBS_NAME_SET Set,
    _In_ PCWSTR Name)
{
    PWSTR* Names;
    PWSTR Copy;
    SIZE_T Length;
    ULONG Capacity, Index;

    for (Index = 0; Index < Set->Count; Index++)
    {
        if (CompareStringOrdinal(Set->Names[Index], -1, Name, -1, TRUE) == CSTR_EQUAL)
        {
            return S_FALSE;
        }
    }
    if (Set->Count == Set->Capacity)
    {
        if (Set->Capacity > MAXULONG / 2)
        {
            return E_OUTOFMEMORY;
        }
        Capacity = Set->Capacity == 0 ? 8 : Set->Capacity * 2;
        Names = Mem_ReAlloc(Set->Names, (SIZE_T)Capacity * sizeof(PWSTR));
        if (Names == NULL)
        {
            return E_OUTOFMEMORY;
        }
        Set->Names = Names;
        Set->Capacity = Capacity;
    }
    Length = Str_SizeW(Name) + sizeof(UNICODE_NULL);
    Copy = Mem_Alloc(Length);
    if (Copy == NULL)
    {
        return E_OUTOFMEMORY;
    }
    RtlCopyMemory(Copy, Name, Length);
    Set->Names[Set->Count++] = Copy;
    return S_OK;
}

HRESULT
NTAPI
Sys_CbsOpenSession(
    _In_ PCWSTR ClientId,
    _Outptr_ ICbsSession** Session)
{
    ICbsSession* Value;
    HRESULT Hr;

    Hr = CoCreateInstance(&CLSID_CbsSession,
                          NULL,
                          CLSCTX_LOCAL_SERVER,
                          &IID_ICbsSession,
                          (PVOID*)&Value);
    if (FAILED(Hr))
    {
        return Hr;
    }
    Hr = CoSetProxyBlanket((IUnknown*)Value,
                           RPC_C_AUTHN_DEFAULT,
                           RPC_C_AUTHZ_NONE,
                           NULL,
                           RPC_C_AUTHN_LEVEL_CALL,
                           RPC_C_IMP_LEVEL_IMPERSONATE,
                           NULL,
                           EOAC_NONE);
    if (SUCCEEDED(Hr))
    {
        Hr = Value->lpVtbl->Initialize(Value, CbsSessionOptionNone, ClientId, NULL, NULL);
    }
    if (FAILED(Hr))
    {
        Value->lpVtbl->Release(Value);
        return Hr;
    }
    *Session = Value;
    return Hr;
}

HRESULT
NTAPI
Sys_CbsCancelSession(
    _In_ ICbsSession* Session,
    _Out_ CBS_REQUIRED_ACTION* RequiredAction)
{
    ICbsSession7* Session7;
    HRESULT Hr;

    Hr = Session->lpVtbl->QueryInterface(Session, &IID_ICbsSession7, (PVOID*)&Session7);
    if (FAILED(Hr))
    {
        return Hr;
    }
    Hr = Session7->lpVtbl->FinalizeEx(Session7,
                                      CBS_SESSION_FINALIZE_OPTION_CANCEL_PENDING,
                                      RequiredAction);
    Session7->lpVtbl->Release(Session7);
    return Hr;
}

HRESULT
NTAPI
Sys_CbsOpenPackage(
    _In_ ICbsSession* Session,
    _In_ PCWSTR PackageIdentity,
    _Outptr_ ICbsPackage** Package)
{
    ICbsIdentity* Identity;
    IUnknown* Unknown;
    HRESULT Hr;

    Hr = Session->lpVtbl->CreateCbsIdentity(Session, &Identity);
    if (FAILED(Hr))
    {
        return Hr;
    }
    Hr = Identity->lpVtbl->LoadFromStringId(Identity, PackageIdentity);
    if (FAILED(Hr))
    {
        goto _Exit;
    }
    Hr = Session->lpVtbl->OpenPackage(Session, 0, Identity, NULL, &Unknown);
    if (FAILED(Hr))
    {
        goto _Exit;
    }
    Hr = Unknown->lpVtbl->QueryInterface(Unknown, &IID_ICbsPackage, (PVOID*)Package);
    Unknown->lpVtbl->Release(Unknown);

_Exit:
    Identity->lpVtbl->Release(Identity);
    return Hr;
}

HRESULT
NTAPI
Sys_CbsGetFeatureState(
    _In_ ICbsUpdate* Update,
    _Out_ PSYS_CBS_FEATURE_STATE State)
{
    HRESULT Hr;

    Hr = Update->lpVtbl->GetCapability(Update, &State->Applicability, &State->Selectability);
    if (FAILED(Hr))
    {
        return Hr;
    }
    return Update->lpVtbl->GetInstallState(Update,
                                            &State->Current,
                                            &State->Intended,
                                            &State->Requested);
}

static
HRESULT
NTAPI
Sys_CbsEnumerateFeatureParents(
    _In_ PCSYS_CBS_FEATURE Feature,
    _In_ __callback PSYS_CBS_FEATURE_PARENT_CALLBACK Callback,
    _In_opt_ PVOID Context)
{
    PWSTR ParentName, ParentSet;
    UINT Index;
    HRESULT Hr;

    for (Index = 0;; Index++)
    {
        Hr = Feature->Update->lpVtbl->GetParentUpdate(Feature->Update,
                                                      Index,
                                                      &ParentName,
                                                      &ParentSet);
        if (Hr == CBS_E_ARRAY_MISSING_INDEX)
        {
            return S_OK;
        }
        if (FAILED(Hr))
        {
            return Hr;
        }
        Hr = Callback(Feature, ParentName, ParentSet, Context);
        CoTaskMemFree(ParentSet);
        CoTaskMemFree(ParentName);
        if (Hr != S_OK)
        {
            return Hr;
        }
    }
}

static
HRESULT
NTAPI
Sys_CbsEnumerateFeature(
    _In_ ICbsUpdate* Update,
    _In_ __callback PSYS_CBS_FEATURE_CALLBACK FeatureCallback,
    _In_opt_ __callback PSYS_CBS_FEATURE_PARENT_CALLBACK ParentCallback,
    _In_opt_ PVOID Context)
{
    SYS_CBS_FEATURE Feature;
    HRESULT Hr;

    Feature.Update = Update;
    Hr = Update->lpVtbl->GetProperty(Update, CbsUpdatePropertyName, (PWSTR*)&Feature.Name);
    if (FAILED(Hr))
    {
        return Hr;
    }
    Hr = Update->lpVtbl->GetProperty(Update,
                                     CbsUpdatePropertyDisplayName,
                                     (PWSTR*)&Feature.DisplayName);
    if (FAILED(Hr))
    {
        goto _Free_Name;
    }
    Hr = Update->lpVtbl->GetProperty(Update,
                                     CbsUpdatePropertyDescription,
                                     (PWSTR*)&Feature.Description);
    if (FAILED(Hr))
    {
        goto _Free_DisplayName;
    }
    Hr = Sys_CbsGetFeatureState(Update, &Feature.State);
    if (SUCCEEDED(Hr))
    {
        Hr = FeatureCallback(&Feature, Context);
    }
    if (Hr == S_OK && ParentCallback != NULL)
    {
        Hr = Sys_CbsEnumerateFeatureParents(&Feature, ParentCallback, Context);
    }
    CoTaskMemFree((PVOID)Feature.Description);

_Free_DisplayName:
    CoTaskMemFree((PVOID)Feature.DisplayName);

_Free_Name:
    CoTaskMemFree((PVOID)Feature.Name);
    return Hr;
}

HRESULT
NTAPI
Sys_CbsEnumeratePackageFeatures(
    _In_ ICbsPackage* Package,
    _In_ CBS_APPLICABILITY Applicability,
    _In_ CBS_SELECTABILITY Selectability,
    _In_ __callback PSYS_CBS_FEATURE_CALLBACK FeatureCallback,
    _In_opt_ __callback PSYS_CBS_FEATURE_PARENT_CALLBACK ParentCallback,
    _In_opt_ PVOID Context)
{
    IEnumCbsUpdate* Enumerator;
    ICbsUpdate* Update;
    ULONG Fetched;
    HRESULT Hr;

    Hr = Package->lpVtbl->EnumerateUpdates(Package, Applicability, Selectability, &Enumerator);
    if (FAILED(Hr))
    {
        return Hr;
    }
    for (;;)
    {
        Hr = Enumerator->lpVtbl->Next(Enumerator, 1, &Update, &Fetched);
        if (Hr == S_FALSE)
        {
            Hr = S_OK;
            break;
        }
        if (FAILED(Hr))
        {
            break;
        }
        Hr = Sys_CbsEnumerateFeature(Update, FeatureCallback, ParentCallback, Context);
        Update->lpVtbl->Release(Update);
        if (Hr != S_OK)
        {
            break;
        }
    }
    Enumerator->lpVtbl->Release(Enumerator);
    return Hr;
}

static
HRESULT
NTAPI
Sys_CbsQueueFeature(
    _In_ ICbsPackage* Package,
    _In_ ICbsUpdate* Update,
    _In_ PCWSTR Name,
    _In_ CBS_INSTALL_STATE State,
    _In_ LOGICAL EnableDependencies,
    _Inout_ PSYS_CBS_NAME_SET Visited)
{
    ICbsUpdate* Parent;
    PWSTR ParentName, ParentSet;
    CBS_INSTALL_STATE Current, Intended, Requested;
    UINT Index;
    HRESULT Hr;

    Hr = Sys_CbsAddName(Visited, Name);
    if (Hr == S_FALSE)
    {
        return S_OK;
    }
    if (FAILED(Hr))
    {
        return Hr;
    }
    if (EnableDependencies)
    {
        for (Index = 0;; Index++)
        {
            Hr = Update->lpVtbl->GetParentUpdate(Update, Index, &ParentName, &ParentSet);
            if (Hr == CBS_E_ARRAY_MISSING_INDEX)
            {
                break;
            }
            if (FAILED(Hr))
            {
                return Hr;
            }
            Hr = Package->lpVtbl->GetUpdate(Package, ParentName, &Parent);
            CoTaskMemFree(ParentSet);
            if (Hr == CBS_E_UNKNOWN_UPDATE)
            {
                CoTaskMemFree(ParentName);
                continue;
            }
            if (FAILED(Hr))
            {
                CoTaskMemFree(ParentName);
                return Hr;
            }
            Hr = Parent->lpVtbl->GetInstallState(Parent, &Current, &Intended, &Requested);
            if (SUCCEEDED(Hr) &&
                Current != CbsInstallStateInstalled &&
                Current != CbsInstallStatePermanent &&
                Requested != CbsInstallStateInstallRequested)
            {
                Hr = Sys_CbsQueueFeature(Package,
                                         Parent,
                                         ParentName,
                                         CbsInstallStateInstallRequested,
                                         TRUE,
                                         Visited);
            }
            Parent->lpVtbl->Release(Parent);
            CoTaskMemFree(ParentName);
            if (FAILED(Hr))
            {
                return Hr;
            }
        }
    }
    return Update->lpVtbl->SetInstallState(Update, 0, State);
}

HRESULT
NTAPI
Sys_CbsSetPackageFeaturesEnabled(
    _In_ ICbsSession* Session,
    _In_ ICbsPackage* Package,
    _In_reads_(FeatureCount) PCWSTR const* FeatureNames,
    _In_range_(>, 0) ULONG FeatureCount,
    _In_ LOGICAL Enable,
    _In_ LOGICAL EnableDependencies)
{
    SYS_CBS_NAME_SET Visited = { 0 };
    ICbsUpdate* Update;
    CBS_REQUIRED_ACTION RequiredAction;
    ULONG Index;
    HRESULT Hr;

    for (Index = 0; Index < FeatureCount; Index++)
    {
        Hr = Package->lpVtbl->GetUpdate(Package, FeatureNames[Index], &Update);
        if (FAILED(Hr))
        {
            goto _Cancel;
        }
        Hr = Sys_CbsQueueFeature(Package,
                                 Update,
                                 FeatureNames[Index],
                                 Enable ? CbsInstallStateInstallRequested :
                                          CbsInstallStateUninstallRequested,
                                 Enable && EnableDependencies,
                                 &Visited);
        Update->lpVtbl->Release(Update);
        if (FAILED(Hr))
        {
            goto _Cancel;
        }
    }
    Hr = Package->lpVtbl->InitiateChanges(Package,
                                          CBS_PACKAGE_CHANGE_OPTION_UNKNOWN1 |
                                              CBS_PACKAGE_CHANGE_OPTION_UNKNOWN4,
                                          Enable ? CbsInstallStateInstalled : CbsInstallStateDefault,
                                          NULL);
    if (SUCCEEDED(Hr))
    {
        goto _Exit;
    }

_Cancel:
    Sys_CbsCancelSession(Session, &RequiredAction);

_Exit:
    Sys_CbsFreeNameSet(&Visited);
    return Hr;
}

HRESULT
NTAPI
Sys_CbsEnumerateFeatures(
    _In_ PCWSTR ClientId,
    _In_ __callback PSYS_CBS_FEATURE_CALLBACK FeatureCallback,
    _In_opt_ __callback PSYS_CBS_FEATURE_PARENT_CALLBACK ParentCallback,
    _In_opt_ PVOID Context)
{
    ICbsSession* Session;
    ICbsPackage* Package;
    CBS_REQUIRED_ACTION RequiredAction;
    HRESULT InitializeHr, FinalizeHr, Hr;

    InitializeHr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(InitializeHr) && InitializeHr != RPC_E_CHANGED_MODE)
    {
        return InitializeHr;
    }
    Hr = Sys_CbsOpenSession(ClientId, &Session);
    if (FAILED(Hr))
    {
        goto _Uninitialize;
    }
    Hr = Sys_CbsOpenPackage(Session, SYS_CBS_FOUNDATION_PACKAGE, &Package);
    if (SUCCEEDED(Hr))
    {
        Hr = Sys_CbsEnumeratePackageFeatures(
            Package,
            (CBS_APPLICABILITY)(CbsApplicabilityNeedsParent | CbsApplicabilityApplicable),
            CbsSelectabilityAll,
            FeatureCallback,
            ParentCallback,
            Context);
        Package->lpVtbl->Release(Package);
    }
    FinalizeHr = Session->lpVtbl->Finalize(Session, &RequiredAction);
    if (SUCCEEDED(Hr) && FAILED(FinalizeHr))
    {
        Hr = FinalizeHr;
    }
    Session->lpVtbl->Release(Session);

_Uninitialize:
    if (InitializeHr != RPC_E_CHANGED_MODE)
    {
        CoUninitialize();
    }
    return Hr;
}

HRESULT
NTAPI
Sys_CbsSetFeaturesEnabled(
    _In_ PCWSTR ClientId,
    _In_reads_(FeatureCount) PCWSTR const* FeatureNames,
    _In_range_(>, 0) ULONG FeatureCount,
    _In_ LOGICAL Enable,
    _In_ LOGICAL EnableDependencies,
    _Out_ CBS_REQUIRED_ACTION* RequiredAction)
{
    ICbsSession* Session;
    ICbsPackage* Package;
    CBS_REQUIRED_ACTION CleanupAction;
    HRESULT InitializeHr, Hr;

    InitializeHr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(InitializeHr) && InitializeHr != RPC_E_CHANGED_MODE)
    {
        return InitializeHr;
    }
    Hr = Sys_CbsOpenSession(ClientId, &Session);
    if (FAILED(Hr))
    {
        goto _Uninitialize;
    }
    Hr = Sys_CbsOpenPackage(Session, SYS_CBS_FOUNDATION_PACKAGE, &Package);
    if (FAILED(Hr))
    {
        Session->lpVtbl->Finalize(Session, &CleanupAction);
        goto _Release_Session;
    }
    Hr = Sys_CbsSetPackageFeaturesEnabled(Session,
                                          Package,
                                          FeatureNames,
                                          FeatureCount,
                                          Enable,
                                          EnableDependencies);
    Package->lpVtbl->Release(Package);
    if (SUCCEEDED(Hr))
    {
        Hr = Session->lpVtbl->Finalize(Session, RequiredAction);
    }

_Release_Session:
    Session->lpVtbl->Release(Session);

_Uninitialize:
    if (InitializeHr != RPC_E_CHANGED_MODE)
    {
        CoUninitialize();
    }
    return Hr;
}

HRESULT
NTAPI
Sys_CbsSetFeatureEnabled(
    _In_ PCWSTR ClientId,
    _In_ PCWSTR FeatureName,
    _In_ LOGICAL Enable,
    _In_ LOGICAL EnableDependencies,
    _Out_ CBS_REQUIRED_ACTION* RequiredAction)
{
    PCWSTR FeatureNames[] = { FeatureName };

    return Sys_CbsSetFeaturesEnabled(ClientId,
                                     FeatureNames,
                                     ARRAYSIZE(FeatureNames),
                                     Enable,
                                     EnableDependencies,
                                     RequiredAction);
}
