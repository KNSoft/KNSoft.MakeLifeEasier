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
    IVirtualDesktop* pVD;
    IApplicationViewCollection* pAVC;
    IApplicationView* pAV;

    hr = Shell_CreateImmersiveISP(&pSP);
    TEST_OK(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        CoUninitialize();
        return;
    }

    hr = Shell_CreateIVirtualDesktopManagerInternal(pSP, &pVDMI);
    TEST_OK(SUCCEEDED(hr));
    if (SUCCEEDED(hr))
    {
        pVDMI->lpVtbl->Release(pVDMI);
    }

    hr = Shell_CreateIApplicationViewCollection(pSP, &pAVC);
    TEST_OK(SUCCEEDED(hr));
    if (SUCCEEDED(hr))
    {
        pAVC->lpVtbl->Release(pAVC);
    }

    pSP->lpVtbl->Release(pSP);
    CoUninitialize();
}
