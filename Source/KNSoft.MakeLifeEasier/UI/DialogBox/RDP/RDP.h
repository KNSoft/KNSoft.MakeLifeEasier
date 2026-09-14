#pragma once

#include "../../../MakeLifeEasier.h"

#ifdef __cplusplus
namespace MSTSCLib
{
    struct IMsRdpClient9;
}
#endif

EXTERN_C_START

typedef struct _UI_RDP_CONTEXT UI_RDP_CONTEXT, *PUI_RDP_CONTEXT;

/* Native IMsTscAxEvents callback, synchronous on the window thread after State has been updated. */
/* Parameters are borrowed for this call only; native by-reference arguments may be written back. */
/* Defer window destruction with PostMessage(WM_CLOSE); Context must remain valid until WM_DESTROY. */
typedef VOID (CALLBACK* UI_RDP_CALLBACK)(
    _Inout_ PUI_RDP_CONTEXT Data,
    _In_ DISPID Id,
    _In_ DISPPARAMS* Parameters,
    _In_opt_ PVOID Context);

/* Input fields, consumed during UI_CreateRdpDialog; callback values are copied to UI_RDP_CONTEXT. */
typedef struct _UI_RDP_OPTIONS
{
    HWND Parent;
    PCWSTR Title;
    DWORD Style;
    DWORD ExStyle;
    RECT Rect;
    UI_RDP_CALLBACK Callback;
    PVOID Context;
} UI_RDP_OPTIONS, *PUI_RDP_OPTIONS;

typedef struct _UI_RDP_STATE
{
    BOOLEAN Connected;
    BOOLEAN ConnectPending;
    ULONG DesktopWidth;
    ULONG DesktopHeight;
    LONG DisconnectReason;
} UI_RDP_STATE, *PUI_RDP_STATE;

/* Allocated by UI_CreateRdpDialog and freed during WM_NCDESTROY. Use on the window thread only. */
struct _UI_RDP_CONTEXT
{
    /* Outputs, read-only. Window operations and Client methods may be called directly. */
    HWND Window;
    HWND HostWindow;
    /* Borrowed reference: do not replace or Release. Import MSTSCLib to use the C++ interface. */
#ifdef __cplusplus
    MSTSCLib::IMsRdpClient9* Client;
#else
    struct IMsRdpClient9* Client;
#endif
    UI_RDP_STATE State;
    /* Output, read-only. Latest display-update attempt; S_FALSE before any attempt. */
    HRESULT DisplayResult;
    /* Output, read-only. Change through UI_RdpDialogSetAutoResize to update the display and retries. */
    BOOLEAN AutoResize;

    /* Inputs copied from Options; caller-writable on the window thread. */
    UI_RDP_CALLBACK Callback;
    PVOID CallbackContext;

    /* Internal bookkeeping; callers must not modify or release these fields. */
    struct IConnectionPoint* ConnectionPoint;
    struct IOleInPlaceActiveObject* ActiveObject;
    IUnknown* EventSink;
    DWORD AdviseCookie;
    SIZE DisplaySize;
    ULONG DisplayDpi;
    ULONG ResizeRetries;
    BOOLEAN Resizing;
};

/* The caller initializes OLE on the window thread and owns its message loop and DPI awareness. */
/* Rect is the outer window rectangle. Style controls visibility and top-level/child window behavior. */
/* Returns after creation. Configure Client before connecting; DestroyWindow(Data->Window) releases the context. */
/* COM references are released during WM_DESTROY; observe window lifetime with standard window messages. */
MLE_API
HRESULT
NTAPI
UI_CreateRdpDialog(
    _In_ const UI_RDP_OPTIONS* Options,
    _Out_ PUI_RDP_CONTEXT* Context);

MLE_API
HRESULT
NTAPI
UI_RdpDialogConnect(
    _Inout_ PUI_RDP_CONTEXT Data);

/* Disabled initially. SmartSizing must be disabled by the caller when enabling auto-resize. */
MLE_API
VOID
NTAPI
UI_RdpDialogSetAutoResize(
    _Inout_ PUI_RDP_CONTEXT Data,
    _In_ BOOLEAN Enabled);

/* Call before IsDialogMessage/TranslateMessage; TRUE means that the client consumed the message. */
MLE_API
BOOL
NTAPI
UI_RdpDialogTranslateMessage(
    _In_ PUI_RDP_CONTEXT Data,
    _Inout_ PMSG Message);

EXTERN_C_END
