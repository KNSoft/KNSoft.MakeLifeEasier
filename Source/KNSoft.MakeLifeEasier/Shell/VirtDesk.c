#include "../MakeLifeEasier.inl"

static
HRESULT
TryCreateImmersiveShellInterface(
    _In_ IServiceProvider* ImmersiveShellISP,
    _In_opt_ REFGUID ServiceGuid,
    _In_reads_(IIDCount) REFIID* IIDs,
    _In_range_(>, 0) ULONG IIDCount,
    _Inout_ LPIID* CachedIID,
    _Outptr_ PVOID* InterfacePtr)
{
    HRESULT hr;

    if (*CachedIID == NULL)
    {
        for (ULONG i = 0; i < IIDCount; i++)
        {
            hr = ImmersiveShellISP->lpVtbl->QueryService(ImmersiveShellISP,
                                                         ServiceGuid != NULL ? ServiceGuid : IIDs[i],
                                                         IIDs[i],
                                                         InterfacePtr);
            if (SUCCEEDED(hr))
            {
                *CachedIID = IIDs[i];
                break;
            }
        }
        return hr;
    }
    return ImmersiveShellISP->lpVtbl->QueryService(ImmersiveShellISP,
                                                   ServiceGuid != NULL ? ServiceGuid : *CachedIID,
                                                   *CachedIID,
                                                   InterfacePtr);
}

HRESULT
NTAPI
Shell_CreateImmersiveISP(
    _Outptr_ IServiceProvider** ImmersiveShellISP)
{
    return CoCreateInstance(&CLSID_ImmersiveShell,
                            NULL,
                            CLSCTX_LOCAL_SERVER,
                            &IID_IServiceProvider,
                            ImmersiveShellISP);
}

#pragma region IVirtualDesktopManagerInternal

static CONST IID* g_apiidVDMI[] = {
    &IID_IVirtualDesktopManagerInternal_26100,
    &IID_IVirtualDesktopManagerInternal_25158,
    &IID_IVirtualDesktopManagerInternal_22621,
    &IID_IVirtualDesktopManagerInternal_21313,
    &IID_IVirtualDesktopManagerInternal_20241,
    &IID_IVirtualDesktopManagerInternal_14393,
    &IID_IVirtualDesktopManagerInternal_10240,
    &IID_IVirtualDesktopManagerInternal_10130,
};

static CONST IID* g_piidVDMI = NULL;

HRESULT
NTAPI
Shell_CreateIVirtualDesktopManagerInternal(
    _In_ IServiceProvider* ImmersiveShellISP,
    _Outptr_ IVirtualDesktopManagerInternal** Ptr)
{
    return TryCreateImmersiveShellInterface(ImmersiveShellISP,
                                            &CLSID_VirtualDesktopManagerInternal,
                                            g_apiidVDMI,
                                            ARRAYSIZE(g_apiidVDMI),
                                            &g_piidVDMI,
                                            Ptr);
}

#pragma endregion

#pragma region IApplicationViewCollection

static CONST IID* g_apiidAVC[] = {
    &IID_IApplicationViewCollection,
    &IID_IApplicationViewCollection_0,
};

static CONST IID* g_piidAVC = NULL;

HRESULT
NTAPI
Shell_CreateIApplicationViewCollection(
    _In_ IServiceProvider* ImmersiveShellISP,
    _Outptr_ IApplicationViewCollection** Ptr)
{
    return TryCreateImmersiveShellInterface(ImmersiveShellISP,
                                            NULL,
                                            g_apiidAVC,
                                            ARRAYSIZE(g_apiidAVC),
                                            &g_piidAVC,
                                            Ptr);
}

#pragma endregion
