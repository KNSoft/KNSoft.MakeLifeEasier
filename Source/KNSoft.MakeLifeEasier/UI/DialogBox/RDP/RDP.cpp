#include "../../../MakeLifeEasier.inl"

#include <OcIdl.h>
#include <Ole2.h>
#include <new>

#import "libid:8C11EFA1-92C3-11D1-BC1E-00C04FA31489" version("1.0") \
    raw_interfaces_only named_guids rename_namespace("MSTSCLib") \
    exclude("wireHWND", "_RemotableHandle", "__MIDL_IWinTypes_0009")

#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "OleAut32.lib")

#define UI_RDP_WINDOW_CLASS L"KNSoft.MakeLifeEasier.UI.RDP"
#define UI_RDP_MESSAGE_UPDATE_DISPLAY (WM_APP + 1)
#define UI_RDP_RESIZE_TIMER 1
#define UI_RDP_RETRY_DELAY 500
#define UI_RDP_RESIZE_RETRIES 10

static
VOID
RdpCancelRetry(
    _Inout_ PUI_RDP_CONTEXT Data)
{
    if (Data->ResizeRetries != 0)
    {
        KillTimer(Data->Window, UI_RDP_RESIZE_TIMER);
        Data->ResizeRetries = 0;
        Data->DisplayDpi = 0;
    }
}

class RdpEventSink final : public IDispatch
{
public:
    explicit RdpEventSink(
        _In_ PUI_RDP_CONTEXT Data) : Data(Data)
    {
    }

    VOID DetachWindow(VOID)
    {
        Data = NULL;
    }

    STDMETHODIMP QueryInterface(
        _In_ REFIID InterfaceId,
        _COM_Outptr_ VOID** Object) override
    {
        if (IsEqualIID(InterfaceId, IID_IUnknown) ||
            IsEqualIID(InterfaceId, IID_IDispatch) ||
            IsEqualIID(InterfaceId, MSTSCLib::DIID_IMsTscAxEvents))
        {
            *Object = static_cast<IDispatch*>(this);
            AddRef();
            return S_OK;
        }
        *Object = NULL;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef(VOID) override
    {
        return (ULONG)InterlockedIncrement(&ReferenceCount);
    }

    STDMETHODIMP_(ULONG) Release(VOID) override
    {
        ULONG Count = (ULONG)InterlockedDecrement(&ReferenceCount);

        if (Count == 0)
        {
            delete this;
        }
        return Count;
    }

    STDMETHODIMP GetTypeInfoCount(
        _Out_ UINT* Count) override
    {
        *Count = 0;
        return S_OK;
    }

    STDMETHODIMP GetTypeInfo(
        _In_ UINT,
        _In_ LCID,
        _Outptr_ ITypeInfo**) override
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP GetIDsOfNames(
        _In_ REFIID,
        _In_reads_(NameCount) LPOLESTR*,
        _In_ UINT NameCount,
        _In_ LCID,
        _Out_writes_(NameCount) DISPID*) override
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP Invoke(
        _In_ DISPID Id,
        _In_ REFIID,
        _In_ LCID,
        _In_ WORD,
        _In_ DISPPARAMS* Parameters,
        _Out_opt_ VARIANT*,
        _Out_opt_ EXCEPINFO*,
        _Out_opt_ UINT*) override
    {
        if (Data == NULL)
        {
            return S_OK;
        }
        if (Id == MSTSCAXEVENT_DISPID_CONNECTING)
        {
            RdpCancelRetry(Data);
            Data->State.ConnectPending = TRUE;
        } else if (Id == MSTSCAXEVENT_DISPID_CONNECTED || Id == MSTSCAXEVENT_DISPID_LOGINCOMPLETE)
        {
            Data->State.Connected = TRUE;
            Data->State.ConnectPending = Id != MSTSCAXEVENT_DISPID_LOGINCOMPLETE;
            if (Id == MSTSCAXEVENT_DISPID_LOGINCOMPLETE)
            {
                // Automatic reconnect can complete without OnConnecting. Update outside the COM event callback.
                Data->DisplayDpi = 0;
                if (!PostMessageW(Data->Window, UI_RDP_MESSAGE_UPDATE_DISPLAY, 0, 0))
                {
                    Data->DisplayResult = HRESULT_FROM_WIN32(Err_GetLastError());
                }
            }
        } else if (Id == MSTSCAXEVENT_DISPID_DISCONNECTED || Id == MSTSCAXEVENT_DISPID_FATALERROR)
        {
            Data->State.Connected = FALSE;
            Data->State.ConnectPending = FALSE;
            Data->State.DisconnectReason = Parameters->rgvarg[0].lVal;
            RdpCancelRetry(Data);
        } else if (Id == MSTSCAXEVENT_DISPID_REMOTEDESKTOPSIZECHANGE)
        {
            Data->State.DesktopWidth = Parameters->rgvarg[1].lVal;
            Data->State.DesktopHeight = Parameters->rgvarg[0].lVal;
        }
        if (Data->Callback != NULL)
        {
            Data->Callback(Data, Id, Parameters, Data->CallbackContext);
        }
        return S_OK;
    }

private:
    PUI_RDP_CONTEXT Data;
    volatile LONG ReferenceCount = 1;
};

static
HRESULT
RdpAdvise(
    _Inout_ PUI_RDP_CONTEXT Data)
{
    IConnectionPointContainer* Container;
    IConnectionPoint* Point;
    HRESULT Result;

    Result = Data->Client->QueryInterface(IID_PPV_ARGS(&Container));
    if (FAILED(Result))
    {
        return Result;
    }
    Result = Container->FindConnectionPoint(MSTSCLib::DIID_IMsTscAxEvents, &Point);
    Container->Release();
    if (FAILED(Result))
    {
        return Result;
    }
    Result = Point->Advise(Data->EventSink, &Data->AdviseCookie);
    if (FAILED(Result))
    {
        Point->Release();
        return Result;
    }
    Data->ConnectionPoint = Point;
    return S_OK;
}

static
ULONG
RdpPixelsToMillimeters(
    _In_ ULONG Pixels,
    _In_ ULONG Dpi)
{
    // One inch is 25.4 mm; scale both sides by 10 for integer arithmetic.
    return (ULONG)max(MSTSCAX_MIN_PHYSICAL_SIZE_MM, MulDiv(Pixels, 254, Dpi * 10));
}

static
VOID
RdpUpdateDisplay(
    _Inout_ PUI_RDP_CONTEXT Data,
    _In_ ULONG Retry = 0)
{
    RECT ClientRect;
    ULONG Width, Height, Dpi, Scale;
    HRESULT Result;

    if (!Data->AutoResize || !Data->State.Connected || Data->State.ConnectPending || Data->Resizing ||
        IsIconic(Data->Window))
    {
        RdpCancelRetry(Data);
        return;
    }
    if (!GetClientRect(Data->HostWindow, &ClientRect))
    {
        Result = HRESULT_FROM_WIN32(Err_GetLastError());
        goto Complete;
    }
    if (ClientRect.right <= 0 || ClientRect.bottom <= 0)
    {
        RdpCancelRetry(Data);
        return;
    }
    // RDP display-control requires an even width.
    Width = (ULONG)max(MSTSCAX_MIN_DESKTOP_SIZE, min(ClientRect.right, MSTSCAX_MAX_DESKTOP_SIZE)) & ~1UL;
    Height = (ULONG)max(MSTSCAX_MIN_DESKTOP_SIZE, min(ClientRect.bottom, MSTSCAX_MAX_DESKTOP_SIZE));
    Dpi = GetDpiForWindow(Data->Window);
    if (Retry == 0 && Width == (ULONG)Data->DisplaySize.cx && Height == (ULONG)Data->DisplaySize.cy &&
        Dpi == Data->DisplayDpi)
    {
        return;
    }
    RdpCancelRetry(Data);
    // Cache the submitted size separately from asynchronous desktop-size notifications.
    Data->DisplaySize.cx = Width;
    Data->DisplaySize.cy = Height;
    Data->DisplayDpi = Dpi;
    Scale = (ULONG)max(SCALE_100_PERCENT,
                       min(MulDiv(Dpi, SCALE_100_PERCENT, USER_DEFAULT_SCREEN_DPI), SCALE_500_PERCENT));
    Result = Data->Client->UpdateSessionDisplaySettings(Width,
                                                       Height,
                                                       RdpPixelsToMillimeters(Width, Dpi),
                                                       RdpPixelsToMillimeters(Height, Dpi),
                                                       MSTSCAX_ORIENTATION_LANDSCAPE,
                                                       Scale,
                                                       SCALE_100_PERCENT);
    // Display-control initialization can lag behind OnLoginComplete.
    if (Result == E_UNEXPECTED && Retry < UI_RDP_RESIZE_RETRIES)
    {
        if (SetTimer(Data->Window, UI_RDP_RESIZE_TIMER, UI_RDP_RETRY_DELAY, NULL) != 0)
        {
            Data->ResizeRetries = Retry + 1;
            Data->DisplayResult = Result;
            return;
        }
        Result = HRESULT_FROM_WIN32(Err_GetLastError());
    }

Complete:
    if (Result != S_OK)
    {
        RdpCancelRetry(Data);
        Data->DisplayDpi = 0;
    }
    Data->DisplayResult = Result;
}

static
VOID
RdpReleaseClient(
    _In_ PUI_RDP_CONTEXT Data)
{
    if (Data->EventSink != NULL)
    {
        static_cast<RdpEventSink*>(Data->EventSink)->DetachWindow();
        if (Data->ConnectionPoint != NULL)
        {
            Data->ConnectionPoint->Unadvise(Data->AdviseCookie);
            Data->ConnectionPoint->Release();
        }
        Data->EventSink->Release();
    }
    if (Data->State.Connected || Data->State.ConnectPending)
    {
        Data->Client->Disconnect();
    }
    if (Data->ActiveObject != NULL)
    {
        Data->ActiveObject->Release();
    }
    if (Data->Client != NULL)
    {
        Data->Client->Release();
    }
}

static
LRESULT
CALLBACK
RdpWindowProc(
    _In_ HWND Window,
    _In_ UINT Message,
    _In_ WPARAM WParam,
    _In_ LPARAM LParam)
{
    PUI_RDP_CONTEXT Data = (PUI_RDP_CONTEXT)GetWindowLongPtrW(Window, GWLP_USERDATA);

    if (Data != NULL)
    {
        if (Message == WM_DPICHANGED)
        {
            const RECT* Rect = (const RECT*)LParam;

            SetWindowPos(Window,
                         NULL,
                         Rect->left,
                         Rect->top,
                         Rect->right - Rect->left,
                         Rect->bottom - Rect->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
            RdpUpdateDisplay(Data);
            return 0;
        } else if (Message == WM_DPICHANGED_AFTERPARENT)
        {
            RdpUpdateDisplay(Data);
            return 0;
        } else if (Message == WM_ENTERSIZEMOVE)
        {
            Data->Resizing = TRUE;
            RdpCancelRetry(Data);
            return 0;
        } else if (Message == WM_EXITSIZEMOVE)
        {
            Data->Resizing = FALSE;
            RdpUpdateDisplay(Data);
            return 0;
        } else if (Message == WM_TIMER && WParam == UI_RDP_RESIZE_TIMER && Data->ResizeRetries != 0)
        {
            RdpUpdateDisplay(Data, Data->ResizeRetries);
            return 0;
        } else if (Message == WM_SIZE)
        {
            if (WParam == SIZE_MINIMIZED)
            {
                RdpCancelRetry(Data);
                return 0;
            }
            if (Data->HostWindow != NULL)
            {
                MoveWindow(Data->HostWindow, 0, 0, LOWORD(LParam), HIWORD(LParam), TRUE);
                RdpUpdateDisplay(Data);
            }
            return 0;
        } else if (Message == WM_SETFOCUS)
        {
            if (Data->HostWindow != NULL)
            {
                SetFocus(Data->HostWindow);
            }
            return 0;
        } else if (Message == UI_RDP_MESSAGE_UPDATE_DISPLAY)
        {
            RdpUpdateDisplay(Data);
            return 0;
        } else if (Message == WM_DESTROY)
        {
            RdpCancelRetry(Data);
            RdpReleaseClient(Data);
            return 0;
        } else if (Message == WM_NCDESTROY)
        {
            LRESULT Result;

            SetWindowLongPtrW(Window, GWLP_USERDATA, 0);
            Result = DefWindowProcW(Window, Message, WParam, LParam);
            Mem_Free(Data);
            return Result;
        }
    }
    return DefWindowProcW(Window, Message, WParam, LParam);
}

HRESULT
NTAPI
UI_CreateRdpDialog(
    _In_ const UI_RDP_OPTIONS* Options,
    _Out_ PUI_RDP_CONTEXT* Context)
{
    WNDCLASSEXW WindowClass = { sizeof(WindowClass) };
    DECLARE_UNICODE_STRING_SIZE(ControlClass, RTL_GUID_STRING_SIZE + 1);
    PUI_RDP_CONTEXT Data;
    IUnknown* Control;
    RECT Rect;
    HRESULT Result;
    W32ERROR Error;

    if (!UI_AxHostInitialize())
    {
        return E_FAIL;
    }
    WindowClass.lpfnWndProc = RdpWindowProc;
    WindowClass.hInstance = (HINSTANCE)&__ImageBase;
    WindowClass.hCursor = (HCURSOR)LoadImageW(NULL,
                                            IDC_ARROW,
                                            IMAGE_CURSOR,
                                            0,
                                            0,
                                            LR_DEFAULTSIZE | LR_SHARED);
    WindowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    WindowClass.lpszClassName = UI_RDP_WINDOW_CLASS;
    if (RegisterClassExW(&WindowClass) == 0)
    {
        Error = Err_GetLastError();
        if (Error != ERROR_CLASS_ALREADY_EXISTS)
        {
            return HRESULT_FROM_WIN32(Error);
        }
    }
    if (!Mem_AllocPtr(Data))
    {
        return E_OUTOFMEMORY;
    }
    RtlZeroMemory(Data, sizeof(*Data));
    Data->DisplayResult = S_FALSE;
    Data->Window = CreateWindowExW(Options->ExStyle,
                                   UI_RDP_WINDOW_CLASS,
                                   Options->Title,
                                   Options->Style | WS_CLIPCHILDREN,
                                   Options->Rect.left,
                                   Options->Rect.top,
                                   Options->Rect.right - Options->Rect.left,
                                   Options->Rect.bottom - Options->Rect.top,
                                   Options->Parent,
                                   NULL,
                                   (HINSTANCE)&__ImageBase,
                                   NULL);
    if (Data->Window == NULL)
    {
        Result = HRESULT_FROM_WIN32(Err_GetLastError());
        Mem_Free(Data);
        return Result;
    }
    SetWindowLongPtrW(Data->Window, GWLP_USERDATA, (LONG_PTR)Data);
    if (!GetClientRect(Data->Window, &Rect))
    {
        Result = HRESULT_FROM_WIN32(Err_GetLastError());
        goto Cleanup;
    }
    RtlStringFromGUIDEx(&__uuidof(MSTSCLib::MsRdpClient10NotSafeForScripting), &ControlClass, FALSE);
    Data->HostWindow = CreateWindowExW(0,
                                       UI_AxHostClassName(),
                                       ControlClass.Buffer,
                                       WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                       0,
                                       0,
                                       Rect.right,
                                       Rect.bottom,
                                       Data->Window,
                                       NULL,
                                       (HINSTANCE)&__ImageBase,
                                       NULL);
    if (Data->HostWindow == NULL)
    {
        Result = HRESULT_FROM_WIN32(Err_GetLastError());
        goto Cleanup;
    }
    Result = UI_AxHostGetControl(Data->HostWindow, &Control);
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Control->QueryInterface(IID_PPV_ARGS(&Data->Client));
    Control->Release();
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Data->Client->QueryInterface(IID_PPV_ARGS(&Data->ActiveObject));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Data->EventSink = new (std::nothrow) RdpEventSink(Data);
    if (Data->EventSink == NULL)
    {
        Result = E_OUTOFMEMORY;
        goto Cleanup;
    }
    Result = RdpAdvise(Data);
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Data->Callback = Options->Callback;
    Data->CallbackContext = Options->Context;
    *Context = Data;
    return S_OK;

Cleanup:
    DestroyWindow(Data->Window);
    return Result;
}

HRESULT
NTAPI
UI_RdpDialogConnect(
    _Inout_ PUI_RDP_CONTEXT Data)
{
    BOOLEAN ConnectPending = Data->State.ConnectPending;
    HRESULT Result;

    Data->State.ConnectPending = TRUE;
    Result = Data->Client->Connect();
    if (FAILED(Result))
    {
        Data->State.ConnectPending = ConnectPending;
    } else
    {
        SetFocus(Data->HostWindow);
    }
    return Result;
}

VOID
NTAPI
UI_RdpDialogSetAutoResize(
    _Inout_ PUI_RDP_CONTEXT Data,
    _In_ BOOLEAN Enabled)
{
    if (Enabled && !Data->AutoResize)
    {
        Data->DisplayDpi = 0;
    }
    Data->AutoResize = Enabled;
    RdpUpdateDisplay(Data);
}

BOOL
NTAPI
UI_RdpDialogTranslateMessage(
    _In_ PUI_RDP_CONTEXT Data,
    _Inout_ PMSG Message)
{
    HWND Focus;

    if (Message->message < WM_KEYFIRST || Message->message > WM_KEYLAST)
    {
        return FALSE;
    }
    Focus = GetFocus();
    return (Focus == Data->HostWindow || IsChild(Data->HostWindow, Focus)) &&
           Data->ActiveObject->TranslateAccelerator(Message) == S_OK;
}
