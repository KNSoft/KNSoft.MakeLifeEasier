#include "ChildSessionAxHost.h"

// Keep SDK-only ATL headers separate from NDK's typed Windows declarations.
#include <atlbase.h>
#include <atlhost.h>

class ChildSessionAxModule final : public ATL::CAtlModuleT<ChildSessionAxModule>
{
};

BOOL
WINAPI
ChildSession_AxHostInitialize(VOID)
{
    static ChildSessionAxModule Module;

    if (ATL::CAtlBaseModule::m_bInitFailed) return FALSE;
    return ATL::AtlAxWinInit();
}

PCWSTR
WINAPI
ChildSession_AxHostClassName(VOID)
{
    return ATL::CAxWindow::GetWndClassName();
}

HRESULT
WINAPI
ChildSession_AxHostGetControl(
    _In_ HWND Window,
    _COM_Outptr_ IUnknown** Control)
{
    return ATL::AtlAxGetControl(Window, Control);
}
