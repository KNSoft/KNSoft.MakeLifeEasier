#include "Test.h"

TEST_FUNC(ShellObject)
{
    HRESULT hr;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        TEST_SKIP("CoInitializeEx failed with 0x%08lX\n", hr);
        return;
    }

    /* Virtual Desktop Interfaces */

    IServiceProvider* pSP;
    IVirtualDesktopManagerInternal* pVDMI;
    IApplicationViewCollection* pAVC;
    IVirtualDesktopPinnedApps* pVDPA;

    hr = Shell_CreateImmersiveISP(&pSP);
    TEST_OK(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        CoUninitialize();
        return;
    }

    hr = Shell_CreateIVirtualDesktopManagerInternal(pSP, &pVDMI, NULL);
    TEST_OK(SUCCEEDED(hr));
    if (SUCCEEDED(hr))
    {
        pVDMI->lpVtbl->Release(pVDMI);
    }

    hr = Shell_CreateIApplicationViewCollection(pSP, &pAVC, NULL);
    TEST_OK(SUCCEEDED(hr));
    if (SUCCEEDED(hr))
    {
        pAVC->lpVtbl->Release(pAVC);
    }

    hr = pSP->lpVtbl->QueryService(pSP, &CLSID_VirtualDesktopPinnedApps, &IID_IVirtualDesktopPinnedApps, &pVDPA);
    TEST_OK(SUCCEEDED(hr));
    if (SUCCEEDED(hr))
    {
        pVDPA->lpVtbl->Release(pVDPA);
    }

    pSP->lpVtbl->Release(pSP);
    CoUninitialize();
}
