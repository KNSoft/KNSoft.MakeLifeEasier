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
                *CachedIID = (LPIID)IIDs[i];
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
    _Outptr_ IVirtualDesktopManagerInternal** Ptr,
    _Outptr_opt_ LPIID* EffetiveIID)
{
    HRESULT hr;

    hr = TryCreateImmersiveShellInterface(ImmersiveShellISP,
                                          &CLSID_VirtualDesktopManagerInternal,
                                          g_apiidVDMI,
                                          ARRAYSIZE(g_apiidVDMI),
                                          &g_piidVDMI,
                                          Ptr);
    if (SUCCEEDED(hr) && EffetiveIID != NULL)
    {
        *EffetiveIID = (LPIID)g_piidVDMI;
    }
    return hr;
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
    _Outptr_ IApplicationViewCollection** Ptr,
    _Outptr_opt_ LPIID* EffetiveIID)
{
    HRESULT hr;

    hr = TryCreateImmersiveShellInterface(ImmersiveShellISP,
                                          NULL,
                                          g_apiidAVC,
                                          ARRAYSIZE(g_apiidAVC),
                                          &g_piidAVC,
                                          Ptr);
    if (SUCCEEDED(hr) && EffetiveIID != NULL)
    {
        *EffetiveIID = (LPIID)g_piidVDMI;
    }
    return hr;
}

#pragma endregion

HRESULT
NTAPI
Shell_CreateIVirtualDesktopPinnedApps(
    _In_ IServiceProvider* ImmersiveShellISP,
    _Outptr_ IVirtualDesktopPinnedApps** Ptr)
{
    return ImmersiveShellISP->lpVtbl->QueryService(ImmersiveShellISP,
                                                   &CLSID_VirtualDesktopPinnedApps,
                                                   &IID_IVirtualDesktopPinnedApps,
                                                   Ptr);
}

HRESULT
NTAPI
Shell_EnumVirtualDesktops(
    _In_ IVirtualDesktopManagerInternal* VDMI,
    _In_ PSHELL_ENUM_VIRTUALDESKTOP_PROC Callback,
    _In_opt_ PVOID Context)
{
    HRESULT hr;
    IObjectArray* Desktops;
    UINT Count;
    IVirtualDesktop* Desktop;

    if (!IS_NT_VERSION_GE(NT_VERSION_WIN11_24H2))
    {
        return E_NOTIMPL;
    }
    hr = VDMI->lpVtbl->GetDesktops(VDMI, &Desktops);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = Desktops->lpVtbl->GetCount(Desktops, &Count);
    if (FAILED(hr))
    {
        return hr;
    }
    for (UINT i = 0; i < Count; i++)
    {
        hr = Desktops->lpVtbl->GetAt(Desktops, i, &IID_IVirtualDesktop, &Desktop);
        if (SUCCEEDED(hr))
        {
            if (!Callback(Desktop, i, Context))
            {
                return S_FALSE;
            }
        } else
        {
            return hr;
        }
    }
    return S_OK;
}
