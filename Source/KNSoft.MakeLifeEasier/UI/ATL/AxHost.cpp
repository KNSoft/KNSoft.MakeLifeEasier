// NDK's PSID (SID*) conflicts with SDK fields using MS_PSID (void*) in ATL headers.
// #include "AxHost.h"

#include <atlbase.h>
#include <atlhost.h>

class AxModule final : public ATL::CAtlModuleT<AxModule>
{
};

BOOL
WINAPI
UI_AxHostInitialize(VOID)
{
    static const BOOL ModuleInitialized = []()
    {
        if (ATL::_pAtlModule == NULL)
        {
            static AxModule Module;
        }
        return !ATL::CAtlBaseModule::m_bInitFailed;
    }();

    if (!ModuleInitialized)
    {
        return FALSE;
    }
    return ATL::AtlAxWinInit();
}

PCWSTR
WINAPI
UI_AxHostClassName(VOID)
{
    return ATL::CAxWindow::GetWndClassName();
}

HRESULT
WINAPI
UI_AxHostGetControl(
    _In_ HWND Window,
    _Outptr_ IUnknown** Control)
{
    return ATL::AtlAxGetControl(Window, Control);
}
