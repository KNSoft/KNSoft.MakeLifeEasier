/*
 * ChildSession: Host the Remote Desktop ActiveX control and connect it to a
 * child session on the local machine.
 *
 * Run: "ChildSession.exe", need administrator privilege.
 */

#define MLE_API
#define _USE_COMMCTL60

#include "../../KNSoft.MakeLifeEasier/MakeLifeEasier.h"

#include <stdio.h>
#include <stdlib.h>

#include <ObjBase.h>
#include <OcIdl.h>
#include <OleAuto.h>
#include <Ole2.h>
#include <Strsafe.h>
#include <new>

#include "ChildSessionAxHost.h"

#import "libid:8C11EFA1-92C3-11D1-BC1E-00C04FA31489" version("1.0") \
    raw_interfaces_only named_guids rename_namespace("MSTSCLib") \
    exclude("wireHWND", "_RemotableHandle", "__MIDL_IWinTypes_0009")

#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "OleAut32.lib")

typedef struct _CHILD_SESSION_CONFIGURATION
{
    BOOLEAN Enabled;
    BOOLEAN CredentialDelegation;
    BOOLEAN HelloOnly;
} CHILD_SESSION_CONFIGURATION, *PCHILD_SESSION_CONFIGURATION;

static const UNICODE_STRING g_ChildSessionCredentialKey =
    RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SOFTWARE\\Policies\\Microsoft\\Windows\\CredentialsDelegation");
static const UNICODE_STRING g_ChildSessionCredentialValue = RTL_CONSTANT_STRING(L"2147483647");
static const UNICODE_STRING g_ChildSessionCredentialTarget = RTL_CONSTANT_STRING(L"TERMSRV/localhost");
static const UNICODE_STRING g_ChildSessionDefaultCredentialPolicy =
    RTL_CONSTANT_STRING(L"AllowDefaultCredentials");
static const UNICODE_STRING g_ChildSessionNtlmCredentialPolicy =
    RTL_CONSTANT_STRING(L"AllowDefCredentialsWhenNTLMOnly");
static const UNICODE_STRING g_ChildSessionPasswordlessKey =
    RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\PasswordLess\\Device");
static const UNICODE_STRING g_ChildSessionPasswordlessValue =
    RTL_CONSTANT_STRING(L"DevicePasswordLessBuildVersion");

static
BOOLEAN
ChildSession_SessionIsRegistryValueMissing(
    _In_ NTSTATUS Status)
{
    return Status == STATUS_OBJECT_NAME_NOT_FOUND ||
           Status == STATUS_OBJECT_PATH_NOT_FOUND;
}

static
W32ERROR
ChildSession_SessionStatusToWin32Error(
    _In_ NTSTATUS Status)
{
    return NT_SUCCESS(Status) ? ERROR_SUCCESS : Err_NtStatusToWin32Error(Status);
}

static
NTSTATUS
ChildSession_SessionQueryCredentialPolicy(
    _In_ HANDLE Key,
    _In_ PCUNICODE_STRING Policy,
    _Out_ PBOOLEAN Enabled)
{
    PKEY_VALUE_PARTIAL_INFORMATION Data = NULL;
    HANDLE ListKey;
    ULONG PolicyEnabled;
    NTSTATUS Status;

    *Enabled = FALSE;
    Status = Sys_RegQueryDword(Key, Policy, &PolicyEnabled);
    if (ChildSession_SessionIsRegistryValueMissing(Status)) return STATUS_SUCCESS;
    if (!NT_SUCCESS(Status) || PolicyEnabled != 1) return Status;
    Status = Sys_RegOpenKeyEx(&ListKey, Key, KEY_QUERY_VALUE, Policy);
    if (ChildSession_SessionIsRegistryValueMissing(Status)) return STATUS_SUCCESS;
    if (!NT_SUCCESS(Status)) return Status;
    Status = Sys_RegQueryData(ListKey, &g_ChildSessionCredentialValue, &Data);
    NtClose(ListKey);
    if (ChildSession_SessionIsRegistryValueMissing(Status)) return STATUS_SUCCESS;
    if (NT_SUCCESS(Status))
    {
        *Enabled = Data->Type == REG_SZ &&
                   Data->DataLength == g_ChildSessionCredentialTarget.Length + sizeof(WCHAR) &&
                   RtlEqualMemory(Data->Data,
                                  g_ChildSessionCredentialTarget.Buffer,
                                  g_ChildSessionCredentialTarget.Length) &&
                   *(PCWCHAR)(Data->Data + g_ChildSessionCredentialTarget.Length) == UNICODE_NULL;
    }
    Mem_Free(Data);
    return Status;
}

static
NTSTATUS
ChildSession_SessionSetCredentialPolicy(
    _In_ HANDLE Key,
    _In_ PCUNICODE_STRING Policy,
    _In_ BOOLEAN Enabled)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ListKey;
    ULONG PolicyEnabled = 1;
    NTSTATUS Status;

    InitializeObjectAttributes(&ObjectAttributes,
                               (PUNICODE_STRING)Policy,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               Key,
                               NULL);
    if (Enabled)
    {
        Status = NtSetValueKey(Key,
                               (PUNICODE_STRING)Policy,
                               0,
                               REG_DWORD,
                               &PolicyEnabled,
                               sizeof(PolicyEnabled));
        if (!NT_SUCCESS(Status)) return Status;
        Status = NtCreateKey(&ListKey,
                             KEY_SET_VALUE,
                             &ObjectAttributes,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             NULL);
        if (!NT_SUCCESS(Status)) return Status;
        Status = NtSetValueKey(ListKey,
                               (PUNICODE_STRING)&g_ChildSessionCredentialValue,
                               0,
                               REG_SZ,
                               g_ChildSessionCredentialTarget.Buffer,
                               g_ChildSessionCredentialTarget.Length + sizeof(WCHAR));
    } else
    {
        Status = NtOpenKey(&ListKey, KEY_SET_VALUE, &ObjectAttributes);
        if (ChildSession_SessionIsRegistryValueMissing(Status)) return STATUS_SUCCESS;
        if (!NT_SUCCESS(Status)) return Status;
        Status = NtDeleteValueKey(ListKey, (PUNICODE_STRING)&g_ChildSessionCredentialValue);
        if (ChildSession_SessionIsRegistryValueMissing(Status)) Status = STATUS_SUCCESS;
    }
    NtClose(ListKey);
    return Status;
}

static
W32ERROR
ChildSession_GetChildSessionId(
    _Out_ PULONG SessionId)
{
    ULONG Value;
    W32ERROR Error;

    *SessionId = MAXULONG;
    if (!WinStationGetChildSessionId(&Value))
    {
        Error = Err_GetLastError();
        return Error == ERROR_FILE_NOT_FOUND ? ERROR_NOT_FOUND : Error;
    }
    if (Value == MAXULONG) return ERROR_NOT_FOUND;
    *SessionId = Value;
    return ERROR_SUCCESS;
}

static
W32ERROR
ChildSession_QueryChildSessionInformation(
    _Out_ PWINSTATIONINFORMATION Information)
{
    ULONG SessionId, ReturnLength;
    W32ERROR Error;

    Error = ChildSession_GetChildSessionId(&SessionId);
    if (Error != ERROR_SUCCESS) return Error;
    if (!WinStationQueryInformationW(WINSTATION_CURRENT_SERVER,
                                     SessionId,
                                     WinStationInformation,
                                     Information,
                                     sizeof(*Information),
                                     &ReturnLength))
    {
        Error = Err_GetLastError();
        return Error == ERROR_FILE_NOT_FOUND ? ERROR_NOT_FOUND : Error;
    }
    return ERROR_SUCCESS;
}

static
W32ERROR
ChildSession_QueryChildSessionCredentialDelegation(
    _Out_ PBOOLEAN Enabled)
{
    HANDLE Key;
    BOOLEAN DefaultEnabled, NtlmEnabled;
    NTSTATUS Status;

    *Enabled = FALSE;
    Status = Sys_RegOpenKey(&Key,
                            KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                            &g_ChildSessionCredentialKey);
    if (ChildSession_SessionIsRegistryValueMissing(Status)) return ERROR_SUCCESS;
    if (!NT_SUCCESS(Status)) return ChildSession_SessionStatusToWin32Error(Status);
    Status = ChildSession_SessionQueryCredentialPolicy(Key,
                                               &g_ChildSessionDefaultCredentialPolicy,
                                               &DefaultEnabled);
    if (NT_SUCCESS(Status))
    {
        Status = ChildSession_SessionQueryCredentialPolicy(Key,
                                                   &g_ChildSessionNtlmCredentialPolicy,
                                                   &NtlmEnabled);
    }
    NtClose(Key);
    if (NT_SUCCESS(Status)) *Enabled = DefaultEnabled && NtlmEnabled;
    return ChildSession_SessionStatusToWin32Error(Status);
}

static
W32ERROR
ChildSession_SetChildSessionCredentialDelegation(
    _In_ BOOLEAN Enabled)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Key;
    NTSTATUS Status;

    if (Enabled)
    {
        InitializeObjectAttributes(&ObjectAttributes,
                                   (PUNICODE_STRING)&g_ChildSessionCredentialKey,
                                   OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        Status = NtCreateKey(&Key,
                             KEY_SET_VALUE | KEY_CREATE_SUB_KEY,
                             &ObjectAttributes,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             NULL);
    } else
    {
        Status = Sys_RegOpenKey(&Key,
                                KEY_ENUMERATE_SUB_KEYS,
                                &g_ChildSessionCredentialKey);
        if (ChildSession_SessionIsRegistryValueMissing(Status)) return ERROR_SUCCESS;
    }
    if (!NT_SUCCESS(Status)) return ChildSession_SessionStatusToWin32Error(Status);
    Status = ChildSession_SessionSetCredentialPolicy(Key,
                                             &g_ChildSessionDefaultCredentialPolicy,
                                             Enabled);
    if (NT_SUCCESS(Status))
    {
        Status = ChildSession_SessionSetCredentialPolicy(Key,
                                                 &g_ChildSessionNtlmCredentialPolicy,
                                                 Enabled);
    }
    NtClose(Key);
    return ChildSession_SessionStatusToWin32Error(Status);
}

static
W32ERROR
ChildSession_QueryChildSessionHelloOnly(
    _Out_ PBOOLEAN Enabled)
{
    HANDLE Key;
    ULONG Value;
    NTSTATUS Status;

    *Enabled = FALSE;
    Status = Sys_RegOpenKey(&Key, KEY_QUERY_VALUE, &g_ChildSessionPasswordlessKey);
    if (ChildSession_SessionIsRegistryValueMissing(Status)) return ERROR_SUCCESS;
    if (!NT_SUCCESS(Status)) return ChildSession_SessionStatusToWin32Error(Status);
    Status = Sys_RegQueryDword(Key, &g_ChildSessionPasswordlessValue, &Value);
    NtClose(Key);
    if (ChildSession_SessionIsRegistryValueMissing(Status)) return ERROR_SUCCESS;
    if (NT_SUCCESS(Status)) *Enabled = Value == 2;
    return ChildSession_SessionStatusToWin32Error(Status);
}

static
W32ERROR
ChildSession_SetChildSessionHelloOnly(
    _In_ BOOLEAN Enabled)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Key;
    ULONG Value = Enabled ? 2 : 0;
    NTSTATUS Status;

    InitializeObjectAttributes(&ObjectAttributes,
                               (PUNICODE_STRING)&g_ChildSessionPasswordlessKey,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtCreateKey(&Key,
                         KEY_SET_VALUE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         NULL);
    if (!NT_SUCCESS(Status)) return ChildSession_SessionStatusToWin32Error(Status);
    Status = NtSetValueKey(Key,
                           (PUNICODE_STRING)&g_ChildSessionPasswordlessValue,
                           0,
                           REG_DWORD,
                           &Value,
                           sizeof(Value));
    NtClose(Key);
    return ChildSession_SessionStatusToWin32Error(Status);
}

static
W32ERROR
ChildSession_QueryChildSessionConfiguration(
    _Out_ PCHILD_SESSION_CONFIGURATION Configuration)
{
    W32ERROR Error;

    RtlZeroMemory(Configuration, sizeof(*Configuration));
    if (!WinStationIsChildSessionsEnabled(&Configuration->Enabled))
    {
        return Err_GetLastError();
    }
    Error = ChildSession_QueryChildSessionCredentialDelegation(&Configuration->CredentialDelegation);
    if (Error != ERROR_SUCCESS) return Error;
    return ChildSession_QueryChildSessionHelloOnly(&Configuration->HelloOnly);
}

static
W32ERROR
ChildSession_ValidateChildSessionParent(VOID)
{
    NTSTATUS Status;
    WINSTATIONINFORMATION Information;
    ULONG ReturnLength;

    // This demo edits machine-wide settings; elevation alone is not an administrator check.
    Status = PS_IsCurrentAdminToken();
    if (Status != STATUS_SUCCESS) return Err_NtStatusToWin32Error(Status);
    if (!WinStationQueryInformationW(WINSTATION_CURRENT_SERVER,
                                     WINSTATION_CURRENT_SESSION,
                                     WinStationInformation,
                                     &Information,
                                     sizeof(Information),
                                     &ReturnLength))
    {
        return Err_GetLastError();
    }
    if (Information.ConnectState != State_Active) return ERROR_CTX_WINSTATION_NOT_FOUND;
    return Information.UserName[0] != UNICODE_NULL ? ERROR_SUCCESS : ERROR_NOT_LOGGED_ON;
}

static
W32ERROR
ChildSession_LogoffChildSession(
    _In_ BOOLEAN Wait)
{
    ULONG SessionId;
    W32ERROR Error;

    Error = ChildSession_GetChildSessionId(&SessionId);
    if (Error == ERROR_NOT_FOUND) return ERROR_SUCCESS;
    if (Error != ERROR_SUCCESS) return Error;
    // TRUE waits for reset/disconnect completion; FALSE only submits the request.
    return WinStationReset(WINSTATION_CURRENT_SERVER, SessionId, Wait) ?
               ERROR_SUCCESS :
               Err_GetLastError();
}


#define CHILD_SESSION_CONSOLE_WINDOW_CLASS L"KNSoft.MakeLifeEasier.ChildSession.Console"
#define CHILD_SESSION_RDP_WINDOW_CLASS L"KNSoft.MakeLifeEasier.ChildSession.Rdp"
#define CHILD_SESSION_WINDOW_TITLE L"KNSoft Child Session Console"
#define CHILD_SESSION_RDP_WINDOW_TITLE L"KNSoft Child Session"
#define CHILD_SESSION_HOST_CLASS ChildSession_AxHostClassName()
#define CHILD_SESSION_CONTROL_CLASS L"{A0C63C30-F08D-4AB4-907C-34905D770C7D}"
#define CHILD_SESSION_MESSAGE_EVENT (WM_APP + 1)
#define CHILD_SESSION_MESSAGE_RDP_CLOSE (WM_APP + 2)
#define CHILD_SESSION_MESSAGE_DESKTOP_SIZE (WM_APP + 3)
#define CHILD_SESSION_MESSAGE_DISPLAY_UPDATE (WM_APP + 4)
#define CHILD_SESSION_RESIZE_TIMER 1
#define CHILD_SESSION_RESIZE_DELAY 300
#define CHILD_SESSION_RESIZE_RETRIES 10

#define CHILD_SESSION_BUTTON_REFRESH 1001
#define CHILD_SESSION_BUTTON_CONNECT 1002
#define CHILD_SESSION_BUTTON_DISCONNECT 1003
#define CHILD_SESSION_BUTTON_LOGOFF 1004
#define CHILD_SESSION_BUTTON_CLOSE 1005
#define CHILD_SESSION_BUTTON_CONFIGURATION 1010

typedef HRESULT (WINAPI *CHILD_SESSION_ATL_AX_GET_CONTROL)(
    _In_ HWND Window,
    _Out_ IUnknown** Control);

typedef struct _CHILD_SESSION_RESOLUTION
{
    ULONG Width;
    ULONG Height;
    PCWSTR Name;
} CHILD_SESSION_RESOLUTION, *PCHILD_SESSION_RESOLUTION;

static const CHILD_SESSION_RESOLUTION g_ChildSessionResolutions[] = {
    { 1024, 768, L"1024 x 768" },
    { 1280, 720, L"1280 x 720" },
    { 1366, 768, L"1366 x 768" },
    { 1600, 900, L"1600 x 900" },
    { 1920, 1080, L"1920 x 1080" },
    { 2560, 1440, L"2560 x 1440" }
};

typedef struct _CHILD_SESSION_CONFIGURATION_STATE
{
    CHILD_SESSION_CONFIGURATION Original;
    BOOLEAN Valid;
    BOOLEAN EnabledTouched;
} CHILD_SESSION_CONFIGURATION_STATE, *PCHILD_SESSION_CONFIGURATION_STATE;

class ChildSessionEventSink;

typedef struct _CHILD_SESSION_RDP_WINDOW_STATE
{
    HWND Window;
    HWND HostWindow;
    HWND OwnerWindow;
    IUnknown* Control;
    IDispatch* Client;
    IConnectionPoint* ConnectionPoint;
    IOleInPlaceActiveObject* ActiveObject;
    ChildSessionEventSink* EventSink;
    DWORD AdviseCookie;
    ULONG DesktopWidth;
    ULONG DesktopHeight;
    ULONG DisplayDpi;
    ULONG ResizeRetries;
    BOOLEAN ResizePending;
    BOOLEAN Resizing;
    BOOLEAN Connected;
    BOOLEAN ConnectPending;
} CHILD_SESSION_RDP_WINDOW_STATE, *PCHILD_SESSION_RDP_WINDOW_STATE;

typedef struct _CHILD_SESSION_WINDOW_STATE
{
    HWND ConfigurationText;
    HWND SessionText;
    HWND OperationText;
    HWND ResolutionText;
    HWND ResolutionCombo;
    HWND RefreshButton;
    HWND ConnectButton;
    HWND DisconnectButton;
    HWND LogoffButton;
    HWND CloseButton;
    HWND ConfigurationButtons[6];
    HWND ConfigurationNote;
    HFONT Font;
    CHILD_SESSION_RDP_WINDOW_STATE RdpWindow;
    CHILD_SESSION_ATL_AX_GET_CONTROL AtlAxGetControl;
    PCHILD_SESSION_CONFIGURATION_STATE Configuration;
    LONG DisconnectReason;
    BOOLEAN OwnsChildSession;
    BOOLEAN SessionExists;
} CHILD_SESSION_WINDOW_STATE, *PCHILD_SESSION_WINDOW_STATE;

static
HRESULT
ChildSession_GetProperty(
    _In_ IDispatch* Object,
    _In_ PCWSTR Name,
    _Out_ VARIANT* Value)
{
    LPOLESTR PropertyName = const_cast<LPOLESTR>(Name);
    DISPID Id;
    DISPPARAMS Parameters = { 0 };
    HRESULT Result;

    Result = Object->GetIDsOfNames(IID_NULL,
                                   &PropertyName,
                                   1,
                                   LOCALE_INVARIANT,
                                   &Id);
    if (FAILED(Result)) return Result;
    VariantInit(Value);
    return Object->Invoke(Id,
                          IID_NULL,
                          LOCALE_INVARIANT,
                          DISPATCH_PROPERTYGET,
                          &Parameters,
                          Value,
                          NULL,
                          NULL);
}

static
HRESULT
ChildSession_SetProperty(
    _In_ IDispatch* Object,
    _In_ PCWSTR Name,
    _In_ VARIANT* Value)
{
    LPOLESTR PropertyName = const_cast<LPOLESTR>(Name);
    DISPID Id, PutId = DISPID_PROPERTYPUT;
    DISPPARAMS Parameters = { Value, &PutId, 1, 1 };
    HRESULT Result;

    Result = Object->GetIDsOfNames(IID_NULL,
                                   &PropertyName,
                                   1,
                                   LOCALE_INVARIANT,
                                   &Id);
    return FAILED(Result) ?
               Result :
               Object->Invoke(Id,
                              IID_NULL,
                              LOCALE_INVARIANT,
                              DISPATCH_PROPERTYPUT,
                              &Parameters,
                              NULL,
                              NULL,
                              NULL);
}

static
HRESULT
ChildSession_Invoke(
    _In_ IDispatch* Object,
    _In_ PCWSTR Name)
{
    LPOLESTR MethodName = const_cast<LPOLESTR>(Name);
    DISPID Id;
    DISPPARAMS Parameters = { 0 };
    HRESULT Result;

    Result = Object->GetIDsOfNames(IID_NULL,
                                   &MethodName,
                                   1,
                                   LOCALE_INVARIANT,
                                   &Id);
    return FAILED(Result) ?
               Result :
               Object->Invoke(Id,
                              IID_NULL,
                              LOCALE_INVARIANT,
                              DISPATCH_METHOD,
                              &Parameters,
                              NULL,
                              NULL,
                              NULL);
}

static
HRESULT
ChildSession_GetObject(
    _In_ IDispatch* Object,
    _In_ PCWSTR Name,
    _Out_ IDispatch** Value)
{
    VARIANT Property;
    HRESULT Result;

    *Value = NULL;
    Result = ChildSession_GetProperty(Object, Name, &Property);
    if (FAILED(Result)) return Result;
    if (Property.vt == VT_DISPATCH && Property.pdispVal != NULL)
    {
        *Value = Property.pdispVal;
        (*Value)->AddRef();
        Result = S_OK;
    } else
    {
        Result = Property.vt == VT_UNKNOWN && Property.punkVal != NULL ?
                     Property.punkVal->QueryInterface(IID_PPV_ARGS(Value)) :
                     E_NOINTERFACE;
    }
    VariantClear(&Property);
    return Result;
}

static
HRESULT
ChildSession_SetBoolean(
    _In_ IDispatch* Object,
    _In_ PCWSTR Name,
    _In_ VARIANT_BOOL Value)
{
    VARIANT Property;

    VariantInit(&Property);
    Property.vt = VT_BOOL;
    Property.boolVal = Value;
    return ChildSession_SetProperty(Object, Name, &Property);
}

static
HRESULT
ChildSession_SetLong(
    _In_ IDispatch* Object,
    _In_ PCWSTR Name,
    _In_ LONG Value)
{
    VARIANT Property;

    VariantInit(&Property);
    Property.vt = VT_I4;
    Property.lVal = Value;
    return ChildSession_SetProperty(Object, Name, &Property);
}

static
HRESULT
ChildSession_SetString(
    _In_ IDispatch* Object,
    _In_ PCWSTR Name,
    _In_ PCWSTR Value)
{
    VARIANT Property;
    HRESULT Result;

    VariantInit(&Property);
    Property.vt = VT_BSTR;
    Property.bstrVal = SysAllocString(Value);
    if (Property.bstrVal == NULL) return E_OUTOFMEMORY;
    Result = ChildSession_SetProperty(Object, Name, &Property);
    VariantClear(&Property);
    return Result;
}

static
HRESULT
ChildSession_SetExtendedBoolean(
    _In_ MSTSCLib::IMsRdpExtendedSettings* Settings,
    _In_ PCWSTR Name,
    _In_ VARIANT_BOOL Value)
{
    VARIANT Property;
    BSTR PropertyName;
    HRESULT Result;

    PropertyName = SysAllocString(Name);
    if (PropertyName == NULL) return E_OUTOFMEMORY;
    VariantInit(&Property);
    Property.vt = VT_BOOL;
    Property.boolVal = Value;
    Result = Settings->put_Property(PropertyName, &Property);
    SysFreeString(PropertyName);
    return Result;
}

class ChildSessionEventSink final : public IDispatch
{
public:
    explicit ChildSessionEventSink(
        _In_ HWND Window) : Window(Window)
    {
    }

    VOID SetWindow(
        _In_ HWND Value)
    {
        Window = Value;
    }

    STDMETHODIMP QueryInterface(
        _In_ REFIID InterfaceId,
        _COM_Outptr_ VOID** Object) override
    {
        if (Object == NULL) return E_POINTER;
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

        if (Count == 0) delete this;
        return Count;
    }

    STDMETHODIMP GetTypeInfoCount(
        _Out_ UINT* Count) override
    {
        if (Count == NULL) return E_POINTER;
        *Count = 0;
        return S_OK;
    }

    STDMETHODIMP GetTypeInfo(
        _In_ UINT,
        _In_ LCID,
        _COM_Outptr_ ITypeInfo**) override
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
        LONG Error = 0;

        if (Id == 12 && Parameters != NULL && Parameters->cArgs == 2 &&
            Parameters->rgvarg[1].vt == VT_I4 && Parameters->rgvarg[0].vt == VT_I4 &&
            Parameters->rgvarg[1].lVal > 0 && Parameters->rgvarg[0].lVal > 0)
        {
            PostMessageW(Window,
                         CHILD_SESSION_MESSAGE_DESKTOP_SIZE,
                         (WPARAM)Parameters->rgvarg[1].lVal,
                         (LPARAM)Parameters->rgvarg[0].lVal);
            return S_OK;
        }

        if ((Id == 4 || Id == 10 || Id == 11 || Id == 22) &&
            Parameters != NULL && Parameters->cArgs != 0 &&
            Parameters->rgvarg[0].vt == VT_I4)
        {
            Error = Parameters->rgvarg[0].lVal;
        }
        if (Id == 1 || Id == 2 || Id == 3 || Id == 4 || Id == 10 ||
            Id == 11 || Id == 18 || Id == 19 || Id == 22)
        {
            PostMessageW(Window, CHILD_SESSION_MESSAGE_EVENT, (WPARAM)Id, (LPARAM)Error);
        }
        return S_OK;
    }

private:
    HWND Window;
    volatile LONG ReferenceCount = 1;
};

static
HRESULT
ChildSession_Advise(
    _In_ IUnknown* Control,
    _In_ IUnknown* Sink,
    _Out_ IConnectionPoint** ConnectionPoint,
    _Out_ PDWORD Cookie)
{
    IConnectionPointContainer* Container;
    IConnectionPoint* Point;
    HRESULT Result;

    *ConnectionPoint = NULL;
    *Cookie = 0;
    Result = Control->QueryInterface(IID_PPV_ARGS(&Container));
    if (FAILED(Result)) return Result;
    Result = Container->FindConnectionPoint(MSTSCLib::DIID_IMsTscAxEvents, &Point);
    Container->Release();
    if (FAILED(Result)) return Result;
    Result = Point->Advise(Sink, Cookie);
    if (FAILED(Result))
    {
        Point->Release();
        return Result;
    }
    *ConnectionPoint = Point;
    return S_OK;
}

static
PCWSTR
ChildSession_GetStateName(
    _In_ WINSTATIONSTATECLASS State)
{
    switch (State)
    {
        case State_Active: return L"Active";
        case State_Connected: return L"Connected";
        case State_ConnectQuery: return L"ConnectQuery";
        case State_Shadow: return L"Shadow";
        case State_Disconnected: return L"Disconnected";
        case State_Idle: return L"Idle";
        case State_Listen: return L"Listen";
        case State_Reset: return L"Reset";
        case State_Down: return L"Down";
        case State_Init: return L"Init";
        default: return L"Unknown";
    }
}

static
VOID
ChildSession_SetOperation(
    _In_ PCHILD_SESSION_WINDOW_STATE State,
    _In_ PCWSTR Text)
{
    SetWindowTextW(State->OperationText, Text);
}

static
VOID
ChildSession_SetWin32Error(
    _In_ PCHILD_SESSION_WINDOW_STATE State,
    _In_ PCWSTR Operation,
    _In_ W32ERROR Error)
{
    WCHAR Text[256];

    StringCchPrintfW(Text,
                     ARRAYSIZE(Text),
                     L"%s failed: Win32 error %lu",
                     Operation,
                     Error);
    ChildSession_SetOperation(State, Text);
    printf("%ls failed with Win32 error %lu\n", Operation, Error);
}

static
VOID
ChildSession_SetHResult(
    _In_ PCHILD_SESSION_WINDOW_STATE State,
    _In_ PCWSTR Operation,
    _In_ HRESULT Result)
{
    WCHAR Text[256];

    StringCchPrintfW(Text,
                     ARRAYSIZE(Text),
                     L"%s failed: HRESULT 0x%08lX",
                     Operation,
                     (ULONG)Result);
    ChildSession_SetOperation(State, Text);
    printf("%ls failed with HRESULT 0x%08lX\n", Operation, (ULONG)Result);
}

static
W32ERROR
ChildSession_RefreshWindow(
    _In_ PCHILD_SESSION_WINDOW_STATE State)
{
    CHILD_SESSION_CONFIGURATION Configuration;
    WINSTATIONINFORMATION Information;
    WCHAR Text[384];
    W32ERROR ConfigurationError, SessionError;

    for (ULONG Index = 0; Index < ARRAYSIZE(State->ConfigurationButtons); Index++)
    {
        EnableWindow(State->ConfigurationButtons[Index], !State->RdpWindow.ConnectPending);
    }

    // Session RPCs can wait for logon; keep pumping the ActiveX STA during connection.
    if (State->RdpWindow.ConnectPending)
    {
        SetWindowTextW(State->SessionText,
                       L"Child session  |  Waiting for logon; status query deferred");
        EnableWindow(State->ConnectButton, FALSE);
        EnableWindow(State->DisconnectButton, TRUE);
        EnableWindow(State->LogoffButton, FALSE);
        EnableWindow(State->ResolutionCombo, FALSE);
        return ERROR_SUCCESS;
    }

    ConfigurationError = ChildSession_QueryChildSessionConfiguration(&Configuration);
    if (ConfigurationError == ERROR_SUCCESS)
    {
        StringCchPrintfW(Text,
                         ARRAYSIZE(Text),
                         L"Configuration  |  Child sessions: %s%s  |  Credential delegation: %s  |  Hello-only: %s",
                         Configuration.Enabled ? L"Enabled" : L"Disabled",
                         State->Configuration->EnabledTouched ? L" (temporary)" : L"",
                         Configuration.CredentialDelegation ? L"Enabled" : L"Disabled",
                         Configuration.HelloOnly ? L"Enabled" : L"Disabled");
    } else
    {
        StringCchPrintfW(Text,
                         ARRAYSIZE(Text),
                         L"Configuration unavailable: Win32 error %lu",
                         ConfigurationError);
    }
    SetWindowTextW(State->ConfigurationText, Text);

    SessionError = ChildSession_QueryChildSessionInformation(&Information);
    if (SessionError == ERROR_SUCCESS)
    {
        State->SessionExists = TRUE;
        if (Information.UserName[0] != UNICODE_NULL)
        {
            StringCchPrintfW(Text,
                             ARRAYSIZE(Text),
                             Information.Domain[0] != UNICODE_NULL ?
                                 L"Child session  |  ID: %lu  |  State: %s  |  User: %s\\%s" :
                                 L"Child session  |  ID: %lu  |  State: %s  |  User: %s%s",
                             Information.LogonId,
                             ChildSession_GetStateName(Information.ConnectState),
                             Information.Domain,
                             Information.UserName);
        } else
        {
            StringCchPrintfW(Text,
                             ARRAYSIZE(Text),
                             L"Child session  |  ID: %lu  |  State: %s",
                             Information.LogonId,
                             ChildSession_GetStateName(Information.ConnectState));
        }
    } else if (SessionError == ERROR_NOT_FOUND)
    {
        State->SessionExists = FALSE;
        StringCchCopyW(Text,
                       ARRAYSIZE(Text),
                       L"Child session  |  None - select a resolution and click Create / Connect");
    } else
    {
        State->SessionExists = FALSE;
        StringCchPrintfW(Text,
                         ARRAYSIZE(Text),
                         L"Child session unavailable: Win32 error %lu",
                         SessionError);
    }
    SetWindowTextW(State->SessionText, Text);
    SetWindowTextW(State->ConnectButton,
                   SessionError == ERROR_NOT_FOUND ? L"Create / Connect" : L"Connect");
    EnableWindow(State->ConnectButton,
                 !State->RdpWindow.Connected && !State->RdpWindow.ConnectPending);
    EnableWindow(State->DisconnectButton,
                 State->RdpWindow.Window != NULL);
    EnableWindow(State->LogoffButton, State->SessionExists);
    EnableWindow(State->ResolutionCombo, State->RdpWindow.Window == NULL);

    if (ConfigurationError != ERROR_SUCCESS) return ConfigurationError;
    return SessionError == ERROR_NOT_FOUND ? ERROR_SUCCESS : SessionError;
}

static
W32ERROR
ChildSession_PrepareConfiguration(
    _Out_ PCHILD_SESSION_CONFIGURATION_STATE State)
{
    W32ERROR Error;

    RtlZeroMemory(State, sizeof(*State));
    Error = ChildSession_ValidateChildSessionParent();
    if (Error != ERROR_SUCCESS) return Error;
    Error = ChildSession_QueryChildSessionConfiguration(&State->Original);
    if (Error != ERROR_SUCCESS) return Error;
    State->Valid = TRUE;
    return ERROR_SUCCESS;
}

static
W32ERROR
ChildSession_RestoreConfiguration(
    _In_ PCHILD_SESSION_CONFIGURATION_STATE State)
{
    W32ERROR Error, FirstError = ERROR_SUCCESS;

    if (!State->Valid) return ERROR_SUCCESS;
    if (State->EnabledTouched)
    {
        Error = (WinStationEnableChildSessions(State->Original.Enabled) ? ERROR_SUCCESS : Err_GetLastError());
        if (Error != ERROR_SUCCESS) FirstError = Error;
    }
    return FirstError;
}

static
VOID
ChildSession_RdpWindowSetTitle(
    _In_ PCHILD_SESSION_RDP_WINDOW_STATE State,
    _In_ PCWSTR Suffix)
{
    WCHAR Text[256];

    StringCchPrintfW(Text,
                     ARRAYSIZE(Text),
                     L"%s - %lu x %lu%s",
                     CHILD_SESSION_RDP_WINDOW_TITLE,
                     State->DesktopWidth,
                     State->DesktopHeight,
                     Suffix);
    SetWindowTextW(State->Window, Text);
}

static
HRESULT
ChildSession_RdpWindowLayoutControl(
    _In_ PCHILD_SESSION_RDP_WINDOW_STATE State,
    _In_ BOOLEAN Report)
{
    IOleInPlaceObject* InPlaceObject;
    RECT ClientRect, ControlRect;
    HWND ControlWindow;
    HRESULT Result;

    if (State->Control == NULL || State->HostWindow == NULL) return S_FALSE;
    if (!GetClientRect(State->HostWindow, &ClientRect))
    {
        return HRESULT_FROM_WIN32(Err_GetLastError());
    }
    Result = State->Control->QueryInterface(IID_PPV_ARGS(&InPlaceObject));
    if (FAILED(Result)) return Result;
    if (Report && SUCCEEDED(InPlaceObject->GetWindow(&ControlWindow)) &&
        GetWindowRect(ControlWindow, &ControlRect))
    {
        printf("Remote Desktop control before layout: %ld x %ld; host: %ld x %ld; control DPI: %u\n",
                        ControlRect.right - ControlRect.left,
                        ControlRect.bottom - ControlRect.top,
                        ClientRect.right,
                        ClientRect.bottom,
                        GetDpiForWindow(ControlWindow));
    }
    // OLE position and clipping rectangles use parent-client pixels, not HIMETRIC.
    Result = InPlaceObject->SetObjectRects(&ClientRect, &ClientRect);
    if (Report)
    {
        printf("Remote Desktop SetObjectRects: 0x%08lX\n", (ULONG)Result);
        if (SUCCEEDED(InPlaceObject->GetWindow(&ControlWindow)) &&
            GetWindowRect(ControlWindow, &ControlRect))
        {
            printf("Remote Desktop control after layout: %ld x %ld\n",
                            ControlRect.right - ControlRect.left,
                            ControlRect.bottom - ControlRect.top);
        }
    }
    InPlaceObject->Release();
    return Result;
}

static
VOID
ChildSession_RdpWindowQueueResize(
    _Inout_ PCHILD_SESSION_RDP_WINDOW_STATE State)
{
    if (!State->Connected && !State->ConnectPending) return;
    State->ResizePending = TRUE;
    State->ResizeRetries = 0;
    KillTimer(State->Window, CHILD_SESSION_RESIZE_TIMER);
    if (State->Connected && !State->ConnectPending && !State->Resizing && !IsIconic(State->Window))
    {
        if (SetTimer(State->Window, CHILD_SESSION_RESIZE_TIMER, CHILD_SESSION_RESIZE_DELAY, NULL) == 0)
        {
            PostMessageW(State->OwnerWindow,
                         CHILD_SESSION_MESSAGE_DISPLAY_UPDATE,
                         (WPARAM)HRESULT_FROM_WIN32(Err_GetLastError()),
                         0);
        }
    }
}

static
VOID
ChildSession_RdpWindowUpdateDisplay(
    _Inout_ PCHILD_SESSION_RDP_WINDOW_STATE State)
{
    MSTSCLib::IMsRdpClient9* Client = NULL;
    RECT ClientRect;
    ULONG Width, Height, Dpi, Scale;
    SHORT Connected = 0;
    HRESULT Result;

    KillTimer(State->Window, CHILD_SESSION_RESIZE_TIMER);
    if (!State->ResizePending || !State->Connected || State->ConnectPending ||
        State->Resizing || IsIconic(State->Window)) return;
    State->ResizePending = FALSE;
    if (!GetClientRect(State->HostWindow, &ClientRect))
    {
        Result = HRESULT_FROM_WIN32(Err_GetLastError());
        goto Complete;
    }
    if (ClientRect.right <= 0 || ClientRect.bottom <= 0) return;
    // RDP display-control requires an even width in the range 200..8192.
    Width = (ULONG)max(200L, min(ClientRect.right, 8192L)) & ~1UL;
    Height = (ULONG)max(200L, min(ClientRect.bottom, 8192L));
    Dpi = GetDpiForWindow(State->Window);
    if (Dpi == 0) Dpi = USER_DEFAULT_SCREEN_DPI;
    if (Width == State->DesktopWidth && Height == State->DesktopHeight && Dpi == State->DisplayDpi) return;
    Scale = (ULONG)max(100, min(MulDiv(Dpi, 100, USER_DEFAULT_SCREEN_DPI), 500));
    Result = State->Control->QueryInterface(IID_PPV_ARGS(&Client));
    if (SUCCEEDED(Result)) Result = Client->get_Connected(&Connected);
    if (SUCCEEDED(Result))
    {
        Result = Connected == 1 ?
                     Client->UpdateSessionDisplaySettings(Width,
                                                          Height,
                                                          max(10, MulDiv(Width, 254, Dpi * 10)),
                                                          max(10, MulDiv(Height, 254, Dpi * 10)),
                                                          0,
                                                          Scale,
                                                          100) :
                     HRESULT_FROM_WIN32(ERROR_NOT_CONNECTED);
    }
    if (Client != NULL) Client->Release();
    // Display-control initialization can lag behind OnLoginComplete.
    if (Result == E_UNEXPECTED && State->ResizeRetries++ < CHILD_SESSION_RESIZE_RETRIES)
    {
        State->ResizePending = TRUE;
        if (SetTimer(State->Window, CHILD_SESSION_RESIZE_TIMER, 500, NULL) != 0) return;
        State->ResizePending = FALSE;
        Result = HRESULT_FROM_WIN32(Err_GetLastError());
    }
    if (Result == S_OK) State->DisplayDpi = Dpi;
    printf("Remote Desktop dynamic resolution request: %lu x %lu, scale %lu%%: 0x%08lX\n",
                    Width, Height, Scale, (ULONG)Result);

Complete:
    PostMessageW(State->OwnerWindow, CHILD_SESSION_MESSAGE_DISPLAY_UPDATE, (WPARAM)Result, 0);
}

static
LRESULT
CALLBACK
ChildSession_RdpWindowProc(
    _In_ HWND Window,
    _In_ UINT Message,
    _In_ WPARAM WParam,
    _In_ LPARAM LParam)
{
    PCHILD_SESSION_RDP_WINDOW_STATE State =
        (PCHILD_SESSION_RDP_WINDOW_STATE)GetWindowLongPtrW(Window, GWLP_USERDATA);

    if (Message == WM_NCCREATE)
    {
        State = (PCHILD_SESSION_RDP_WINDOW_STATE)((LPCREATESTRUCTW)LParam)->lpCreateParams;
        SetWindowLongPtrW(Window, GWLP_USERDATA, (LONG_PTR)State);
    }
    if (State != NULL)
    {
        switch (Message)
        {
            case WM_DPICHANGED:
            {
                const RECT* SuggestedRect = (const RECT*)LParam;

                SetWindowPos(Window,
                             NULL,
                             SuggestedRect->left,
                             SuggestedRect->top,
                             SuggestedRect->right - SuggestedRect->left,
                             SuggestedRect->bottom - SuggestedRect->top,
                             SWP_NOZORDER | SWP_NOACTIVATE);
                ChildSession_RdpWindowQueueResize(State);
                return 0;
            }
            case WM_ENTERSIZEMOVE:
                State->Resizing = TRUE;
                KillTimer(Window, CHILD_SESSION_RESIZE_TIMER);
                return 0;
            case WM_EXITSIZEMOVE:
                State->Resizing = FALSE;
                if (State->ResizePending) ChildSession_RdpWindowQueueResize(State);
                return 0;
            case WM_TIMER:
                if (WParam == CHILD_SESSION_RESIZE_TIMER)
                {
                    ChildSession_RdpWindowUpdateDisplay(State);
                    return 0;
                }
                break;
            case WM_SIZE:
                if (WParam == SIZE_MINIMIZED)
                {
                    KillTimer(Window, CHILD_SESSION_RESIZE_TIMER);
                    return 0;
                }
                if (State->HostWindow != NULL)
                {
                    MoveWindow(State->HostWindow,
                               0,
                               0,
                               LOWORD(LParam),
                               HIWORD(LParam),
                               TRUE);
                    ChildSession_RdpWindowLayoutControl(State, FALSE);
                    ChildSession_RdpWindowQueueResize(State);
                }
                return 0;
            case WM_SETFOCUS:
                if (State->HostWindow != NULL) SetFocus(State->HostWindow);
                return 0;
            case CHILD_SESSION_MESSAGE_EVENT:
                if (WParam == 1)
                {
                    ChildSession_RdpWindowSetTitle(State, L" - Connecting");
                } else if (WParam == 2 || WParam == 3)
                {
                    State->Connected = TRUE;
                    State->ConnectPending = WParam != 3;
                    if (WParam == 3)
                    {
                        ChildSession_RdpWindowLayoutControl(State, TRUE);
                        if (State->ResizePending) ChildSession_RdpWindowQueueResize(State);
                    }
                    ChildSession_RdpWindowSetTitle(State,
                                                   WParam == 2 ?
                                                       L" - Connected" :
                                                       L" - Active");
                } else if (WParam == 4 || WParam == 10)
                {
                    State->Connected = FALSE;
                    State->ConnectPending = FALSE;
                    State->ResizePending = FALSE;
                    KillTimer(Window, CHILD_SESSION_RESIZE_TIMER);
                    ChildSession_RdpWindowSetTitle(State, L" - Disconnected");
                }
                if (State->OwnerWindow != NULL)
                {
                    PostMessageW(State->OwnerWindow, Message, WParam, LParam);
                }
                return 0;
            case CHILD_SESSION_MESSAGE_DESKTOP_SIZE:
            {
                RECT ClientRect;

                State->DesktopWidth = (ULONG)WParam;
                State->DesktopHeight = (ULONG)LParam;
                ChildSession_RdpWindowSetTitle(State,
                                               State->ConnectPending ? L" - Connecting" : L" - Connected");
                if (GetClientRect(State->HostWindow, &ClientRect))
                {
                    printf("Remote Desktop actual size: %lu x %lu; viewport: %ld x %ld; DPI: %u\n",
                                    State->DesktopWidth,
                                    State->DesktopHeight,
                                    ClientRect.right,
                                    ClientRect.bottom,
                                    GetDpiForWindow(State->HostWindow));
                }
                PostMessageW(State->OwnerWindow, CHILD_SESSION_MESSAGE_DESKTOP_SIZE, WParam, LParam);
                return 0;
            }
            case WM_CLOSE:
                State->ResizePending = FALSE;
                KillTimer(Window, CHILD_SESSION_RESIZE_TIMER);
                if (State->OwnerWindow != NULL)
                {
                    PostMessageW(State->OwnerWindow,
                                 CHILD_SESSION_MESSAGE_RDP_CLOSE,
                                 0,
                                 0);
                } else
                {
                    DestroyWindow(Window);
                }
                return 0;
        }
    }
    return DefWindowProcW(Window, Message, WParam, LParam);
}

static
VOID
ChildSession_RdpWindowDestroy(
    _Inout_ PCHILD_SESSION_RDP_WINDOW_STATE State)
{
    if (State->Window != NULL) KillTimer(State->Window, CHILD_SESSION_RESIZE_TIMER);
    if ((State->Connected || State->ConnectPending) && State->Client != NULL)
    {
        ChildSession_Invoke(State->Client, L"Disconnect");
    }
    if (State->ConnectionPoint != NULL)
    {
        if (State->AdviseCookie != 0)
        {
            State->ConnectionPoint->Unadvise(State->AdviseCookie);
        }
        State->ConnectionPoint->Release();
    }
    if (State->EventSink != NULL)
    {
        State->EventSink->SetWindow(NULL);
        State->EventSink->Release();
    }
    if (State->Window != NULL && IsWindow(State->Window)) DestroyWindow(State->Window);
    if (State->ActiveObject != NULL) State->ActiveObject->Release();
    if (State->Client != NULL) State->Client->Release();
    if (State->Control != NULL) State->Control->Release();
    RtlZeroMemory(State, sizeof(*State));
}

static
HRESULT
ChildSession_RdpWindowCreate(
    _In_ HWND OwnerWindow,
    _In_ const CHILD_SESSION_RESOLUTION* Resolution,
    _In_ CHILD_SESSION_ATL_AX_GET_CONTROL AtlAxGetControl,
    _Out_ PCHILD_SESSION_RDP_WINDOW_STATE State)
{
    IDispatch* Settings = NULL;
    MSTSCLib::IMsRdpExtendedSettings* ExtendedSettings = NULL;
    HMODULE Instance = GetModuleHandleW(NULL);
    RECT WindowRect, ClientRect;
    MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
    LONG WindowWidth, WindowHeight;
    HRESULT Result;

    RtlZeroMemory(State, sizeof(*State));
    State->OwnerWindow = OwnerWindow;
    State->DesktopWidth = Resolution->Width;
    State->DesktopHeight = Resolution->Height;
    SetRect(&WindowRect, 0, 0, (LONG)Resolution->Width, (LONG)Resolution->Height);
    if (!AdjustWindowRectEx(&WindowRect, WS_OVERLAPPEDWINDOW, FALSE, 0))
    {
        return HRESULT_FROM_WIN32(Err_GetLastError());
    }
    State->Window = CreateWindowExW(0,
                                    CHILD_SESSION_RDP_WINDOW_CLASS,
                                    CHILD_SESSION_RDP_WINDOW_TITLE,
                                    WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                    CW_USEDEFAULT,
                                    CW_USEDEFAULT,
                                    WindowRect.right - WindowRect.left,
                                    WindowRect.bottom - WindowRect.top,
                                    NULL,
                                    NULL,
                                    Instance,
                                    State);
    if (State->Window == NULL)
    {
        Result = HRESULT_FROM_WIN32(Err_GetLastError());
        goto Cleanup;
    }
    SetRect(&WindowRect, 0, 0, (LONG)Resolution->Width, (LONG)Resolution->Height);
    if (!AdjustWindowRectExForDpi(&WindowRect,
                                  WS_OVERLAPPEDWINDOW,
                                  FALSE,
                                  0,
                                  GetDpiForWindow(State->Window)) ||
        !GetMonitorInfoW(MonitorFromWindow(State->Window, MONITOR_DEFAULTTONEAREST), &MonitorInfo))
    {
        Result = HRESULT_FROM_WIN32(Err_GetLastError());
        goto Cleanup;
    }
    WindowWidth = min(WindowRect.right - WindowRect.left,
                      MonitorInfo.rcWork.right - MonitorInfo.rcWork.left);
    WindowHeight = min(WindowRect.bottom - WindowRect.top,
                       MonitorInfo.rcWork.bottom - MonitorInfo.rcWork.top);
    if (!SetWindowPos(State->Window,
                      NULL,
                      MonitorInfo.rcWork.left + (MonitorInfo.rcWork.right - MonitorInfo.rcWork.left - WindowWidth) / 2,
                      MonitorInfo.rcWork.top + (MonitorInfo.rcWork.bottom - MonitorInfo.rcWork.top - WindowHeight) / 2,
                      WindowWidth,
                      WindowHeight,
                      SWP_NOZORDER | SWP_NOACTIVATE))
    {
        Result = HRESULT_FROM_WIN32(Err_GetLastError());
        goto Cleanup;
    }
    if (!GetClientRect(State->Window, &ClientRect))
    {
        Result = HRESULT_FROM_WIN32(Err_GetLastError());
        goto Cleanup;
    }
    State->DisplayDpi = GetDpiForWindow(State->Window);
    State->HostWindow = CreateWindowExW(0,
                                        CHILD_SESSION_HOST_CLASS,
                                        CHILD_SESSION_CONTROL_CLASS,
                                        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                        0,
                                        0,
                                        ClientRect.right,
                                        ClientRect.bottom,
                                        State->Window,
                                        NULL,
                                        Instance,
                                        NULL);
    if (State->HostWindow == NULL)
    {
        Result = HRESULT_FROM_WIN32(Err_GetLastError());
        goto Cleanup;
    }
    Result = AtlAxGetControl(State->HostWindow, &State->Control);
    if (SUCCEEDED(Result)) Result = State->Control->QueryInterface(IID_PPV_ARGS(&State->Client));
    if (SUCCEEDED(Result)) Result = State->Control->QueryInterface(IID_PPV_ARGS(&ExtendedSettings));
    if (SUCCEEDED(Result))
    {
        Result = ChildSession_SetExtendedBoolean(ExtendedSettings,
                                                 L"ConnectToChildSession",
                                                 VARIANT_TRUE);
    }
    if (SUCCEEDED(Result))
    {
        Result = ChildSession_SetExtendedBoolean(ExtendedSettings,
                                                 L"EnableFrameBufferRedirection",
                                                 VARIANT_TRUE);
    }
    if (SUCCEEDED(Result)) Result = ChildSession_SetString(State->Client, L"Server", L"localhost");
    if (SUCCEEDED(Result))
    {
        WINSTATIONINFORMATION Information;
        ULONG ReturnLength;

        // Use the parent session's user, including when the demo was run as another administrator.
        if (!WinStationQueryInformationW(WINSTATION_CURRENT_SERVER,
                                         WINSTATION_CURRENT_SESSION,
                                         WinStationInformation,
                                         &Information,
                                         sizeof(Information),
                                         &ReturnLength))
        {
            Result = HRESULT_FROM_WIN32(Err_GetLastError());
        } else if (Information.UserName[0] == UNICODE_NULL)
        {
            Result = HRESULT_FROM_WIN32(ERROR_NOT_LOGGED_ON);
        } else
        {
            Result = ChildSession_SetString(State->Client, L"UserName", Information.UserName);
            if (SUCCEEDED(Result))
            {
                Result = ChildSession_SetString(State->Client, L"Domain", Information.Domain);
            }
        }
    }
    if (SUCCEEDED(Result))
    {
        Result = ChildSession_SetLong(State->Client,
                                      L"DesktopWidth",
                                      (LONG)Resolution->Width);
    }
    if (SUCCEEDED(Result))
    {
        Result = ChildSession_SetLong(State->Client,
                                      L"DesktopHeight",
                                      (LONG)Resolution->Height);
    }
    if (SUCCEEDED(Result)) Result = ChildSession_GetObject(State->Client, L"AdvancedSettings9", &Settings);
    if (SUCCEEDED(Result))
    {
        Result = ChildSession_SetBoolean(Settings, L"EnableCredSspSupport", VARIANT_TRUE);
    }
    if (SUCCEEDED(Result)) Result = ChildSession_SetLong(Settings, L"AuthenticationLevel", 0);
    // Keep the selected desktop resolution without stretching the image.
    if (SUCCEEDED(Result)) Result = ChildSession_SetBoolean(Settings, L"SmartSizing", VARIANT_FALSE);
    if (SUCCEEDED(Result))
    {
        State->EventSink = new (std::nothrow) ChildSessionEventSink(State->Window);
        if (State->EventSink == NULL)
        {
            Result = E_OUTOFMEMORY;
        } else
        {
            Result = ChildSession_Advise(State->Control,
                                          State->EventSink,
                                          &State->ConnectionPoint,
                                          &State->AdviseCookie);
        }
    }
    if (SUCCEEDED(Result))
    {
        Result = State->Control->QueryInterface(IID_PPV_ARGS(&State->ActiveObject));
    }
    if (Settings != NULL)
    {
        Settings->Release();
        Settings = NULL;
    }
    if (ExtendedSettings != NULL)
    {
        ExtendedSettings->Release();
        ExtendedSettings = NULL;
    }
    if (FAILED(Result)) goto Cleanup;
    Result = ChildSession_RdpWindowLayoutControl(State, FALSE);
    if (FAILED(Result)) goto Cleanup;
    ChildSession_RdpWindowSetTitle(State, L"");
    ShowWindow(State->Window, SW_SHOW);
    UpdateWindow(State->Window);
    return S_OK;

Cleanup:
    if (Settings != NULL) Settings->Release();
    if (ExtendedSettings != NULL) ExtendedSettings->Release();
    ChildSession_RdpWindowDestroy(State);
    return Result;
}

static
HRESULT
ChildSession_RdpWindowConnect(
    _Inout_ PCHILD_SESSION_RDP_WINDOW_STATE State)
{
    HRESULT Result;

    State->ConnectPending = TRUE;
    ChildSession_RdpWindowSetTitle(State, L" - Connecting");
    printf("Remote Desktop Connect: entering\n");
    Result = ChildSession_Invoke(State->Client, L"Connect");
    printf("Remote Desktop Connect: returned 0x%08lX\n", (ULONG)Result);
    if (SUCCEEDED(Result))
    {
        SetFocus(State->HostWindow);
    } else
    {
        State->ConnectPending = FALSE;
    }
    return Result;
}

static
const CHILD_SESSION_RESOLUTION*
ChildSession_GetSelectedResolution(
    _In_ PCHILD_SESSION_WINDOW_STATE State)
{
    LRESULT Index = SendMessageW(State->ResolutionCombo, CB_GETCURSEL, 0, 0);

    if (Index < 0 || Index >= ARRAYSIZE(g_ChildSessionResolutions)) Index = 3;
    return &g_ChildSessionResolutions[Index];
}

static
HRESULT
ChildSession_Connect(
    _In_ HWND Window,
    _In_ PCHILD_SESSION_WINDOW_STATE State)
{
    CHILD_SESSION_CONFIGURATION Configuration;
    WINSTATIONINFORMATION Information;
    const CHILD_SESSION_RESOLUTION* Resolution;
    W32ERROR Error;
    HRESULT Result;
    BOOLEAN CreateChildSession = FALSE;

    if (State->RdpWindow.Connected || State->RdpWindow.ConnectPending) return S_FALSE;
    Error = ChildSession_ValidateChildSessionParent();
    if (Error != ERROR_SUCCESS)
    {
        ChildSession_SetWin32Error(State, L"Validate parent session", Error);
        return HRESULT_FROM_WIN32(Error);
    }
    Error = ChildSession_QueryChildSessionConfiguration(&Configuration);
    if (Error != ERROR_SUCCESS)
    {
        ChildSession_SetWin32Error(State, L"Query configuration", Error);
        return HRESULT_FROM_WIN32(Error);
    }
    if (!Configuration.Enabled)
    {
        Error = (WinStationEnableChildSessions(TRUE) ? ERROR_SUCCESS : Err_GetLastError());
        if (Error != ERROR_SUCCESS)
        {
            ChildSession_SetWin32Error(State, L"Enable child sessions", Error);
            return HRESULT_FROM_WIN32(Error);
        }
        State->Configuration->EnabledTouched = TRUE;
    }

    Error = ChildSession_QueryChildSessionInformation(&Information);
    if (Error == ERROR_SUCCESS)
    {
        printf("Connecting to existing child session ID %lu, state %ls\n",
                        Information.LogonId,
                        ChildSession_GetStateName(Information.ConnectState));
        ChildSession_SetOperation(State, L"Connecting to the existing child session...");
    } else if (Error == ERROR_NOT_FOUND)
    {
        CreateChildSession = TRUE;
        printf("Creating and connecting to a child session\n");
        ChildSession_SetOperation(State, L"Creating and connecting to a child session...");
    } else
    {
        ChildSession_SetWin32Error(State, L"Query child session", Error);
        return HRESULT_FROM_WIN32(Error);
    }

    if (State->RdpWindow.Window != NULL)
    {
        ChildSession_RdpWindowDestroy(&State->RdpWindow);
    }
    Resolution = ChildSession_GetSelectedResolution(State);
    Result = ChildSession_RdpWindowCreate(Window,
                                          Resolution,
                                          State->AtlAxGetControl,
                                          &State->RdpWindow);
    if (FAILED(Result))
    {
        ChildSession_SetHResult(State, L"Create Remote Desktop window", Result);
        ChildSession_RefreshWindow(State);
        return Result;
    }
    Result = ChildSession_RdpWindowConnect(&State->RdpWindow);
    if (FAILED(Result))
    {
        ChildSession_SetHResult(State, L"Remote Desktop connect", Result);
        ChildSession_RdpWindowDestroy(&State->RdpWindow);
        ChildSession_RefreshWindow(State);
        return Result;
    }
    State->DisconnectReason = 0;
    if (CreateChildSession) State->OwnsChildSession = TRUE;
    printf("Remote Desktop resolution: %lu x %lu\n",
                    Resolution->Width,
                    Resolution->Height);
    ChildSession_RefreshWindow(State);
    return S_OK;
}

static
HRESULT
ChildSession_Disconnect(
    _In_ HWND Window,
    _In_ PCHILD_SESSION_WINDOW_STATE State)
{
    if (State->RdpWindow.Window == NULL) return S_FALSE;
    ChildSession_RdpWindowDestroy(&State->RdpWindow);
    ChildSession_SetOperation(State, L"Remote Desktop window closed and disconnected");
    printf("Remote Desktop window closed and disconnected\n");
    ChildSession_RefreshWindow(State);
    UNREFERENCED_PARAMETER(Window);
    return S_OK;
}

static
W32ERROR
ChildSession_Logoff(
    _In_ HWND Window,
    _In_ PCHILD_SESSION_WINDOW_STATE State)
{
    W32ERROR Error;

    if (State->RdpWindow.Window != NULL)
    {
        ChildSession_RdpWindowDestroy(&State->RdpWindow);
    }
    Error = ChildSession_LogoffChildSession(TRUE);
    if (Error != ERROR_SUCCESS)
    {
        ChildSession_SetWin32Error(State, L"Log off child session", Error);
        return Error;
    }
    State->OwnsChildSession = FALSE;
    if (State->Configuration->EnabledTouched)
    {
        Error = WinStationEnableChildSessions(State->Configuration->Original.Enabled) ?
                ERROR_SUCCESS : Err_GetLastError();
        if (Error != ERROR_SUCCESS)
        {
            ChildSession_SetWin32Error(State, L"Restore child-session configuration", Error);
            return Error;
        }
        State->Configuration->EnabledTouched = FALSE;
    }
    SetWindowTextW(Window, CHILD_SESSION_WINDOW_TITLE);
    ChildSession_SetOperation(State, L"Child session logged off");
    printf("Child session logged off\n");
    ChildSession_RefreshWindow(State);
    return ERROR_SUCCESS;
}

static
VOID
ChildSession_ChangeConfiguration(
    _In_ HWND Window,
    _In_ PCHILD_SESSION_WINDOW_STATE State,
    _In_ ULONG Index)
{
    W32ERROR Error;
    BOOLEAN Enabled = (Index % 2) == 0;

    if (Index >= ARRAYSIZE(State->ConfigurationButtons) || State->RdpWindow.ConnectPending) return;
    if (MessageBoxW(Window,
                    L"Apply this machine-wide configuration change?\n\n"
                    L"It will remain in effect after this demo closes. Credential delegation permits "
                    L"default credentials for TERMSRV/localhost; disabling Hello-only allows password sign-in.\n\n"
                    L"Removing delegation removes only this helper's entries, not other policy entries. "
                    L"An existing connection may need to be reconnected.",
                    L"Change child-session configuration",
                    MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) return;

    switch (Index / 2)
    {
        case 0:
            Error = (WinStationEnableChildSessions(Enabled) ? ERROR_SUCCESS : Err_GetLastError());
            if (Error == ERROR_SUCCESS)
            {
                // Explicit configuration supersedes the temporary connection-time change.
                State->Configuration->Original.Enabled = Enabled;
                State->Configuration->EnabledTouched = FALSE;
            }
            break;
        case 1:
            Error = ChildSession_SetChildSessionCredentialDelegation(Enabled);
            break;
        default:
            Error = ChildSession_SetChildSessionHelloOnly(Enabled);
            break;
    }
    ChildSession_RefreshWindow(State);
    if (Error != ERROR_SUCCESS)
    {
        ChildSession_SetWin32Error(State, L"Change configuration (refresh shows current state)", Error);
    } else
    {
        ChildSession_SetOperation(State, L"Configuration saved; reconnect if necessary");
    }
}

static
VOID
ChildSession_UpdateConsoleFont(
    _In_ HWND Window,
    _In_ PCHILD_SESSION_WINDOW_STATE State)
{
    UINT Dpi = GetDpiForWindow(Window);
    NONCLIENTMETRICSW Metrics = { sizeof(Metrics) };
    HFONT Font;

    if (!SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(Metrics), &Metrics, 0, Dpi)) return;
    Metrics.lfMessageFont.lfHeight = -max(abs(Metrics.lfMessageFont.lfHeight), MulDiv(10, Dpi, 72));
    Font = CreateFontIndirectW(&Metrics.lfMessageFont);
    if (Font == NULL) return;
    for (HWND Control = GetWindow(Window, GW_CHILD); Control != NULL; Control = GetWindow(Control, GW_HWNDNEXT))
    {
        SendMessageW(Control, WM_SETFONT, (WPARAM)Font, TRUE);
    }
    if (State->Font != NULL) DeleteObject(State->Font);
    State->Font = Font;
}

static
VOID
ChildSession_LayoutWindow(
    _In_ HWND Window,
    _In_ PCHILD_SESSION_WINDOW_STATE State)
{
    RECT ClientRect;
    LONG Width, ButtonLeft;
    const UINT Dpi = GetDpiForWindow(Window);
    const auto Scale = [Dpi](LONG Value) { return MulDiv(Value, Dpi, 96); };
    const LONG Margin = Scale(12), TextHeight = Scale(24), ButtonTop = Scale(240);
    const LONG ButtonWidth = Scale(132), ButtonHeight = Scale(32), ButtonGap = Scale(8);

    if (!GetClientRect(Window, &ClientRect)) return;
    Width = max(ClientRect.right - Margin * 2, 0L);
    ButtonLeft = Margin + Scale(220);
    if (State->ConfigurationText != NULL)
    {
        MoveWindow(State->ConfigurationText, Margin, Scale(10), Width, TextHeight, TRUE);
        MoveWindow(State->SessionText, Margin, Scale(38), Width, TextHeight, TRUE);
        MoveWindow(State->OperationText, Margin, Scale(66), Width, TextHeight, TRUE);
        for (ULONG Index = 0; Index < ARRAYSIZE(State->ConfigurationButtons); Index++)
        {
            MoveWindow(State->ConfigurationButtons[Index],
                       Margin + Scale((Index % 2) * 340), Scale(98 + (Index / 2) * 36),
                       Scale(328), ButtonHeight, TRUE);
        }
        MoveWindow(State->ConfigurationNote, Margin, Scale(208), Width, TextHeight, TRUE);
        MoveWindow(State->ResolutionText, Margin, ButtonTop + Scale(4), Scale(72), TextHeight, TRUE);
        MoveWindow(State->ResolutionCombo, Margin + Scale(76), ButtonTop, Scale(132), Scale(180), TRUE);
        MoveWindow(State->RefreshButton,
                   ButtonLeft,
                   ButtonTop,
                   ButtonWidth,
                   ButtonHeight,
                   TRUE);
        MoveWindow(State->ConnectButton,
                   ButtonLeft + (ButtonWidth + ButtonGap),
                   ButtonTop,
                   ButtonWidth,
                   ButtonHeight,
                   TRUE);
        MoveWindow(State->DisconnectButton,
                   ButtonLeft + (ButtonWidth + ButtonGap) * 2,
                   ButtonTop,
                   ButtonWidth,
                   ButtonHeight,
                   TRUE);
        MoveWindow(State->LogoffButton,
                   ButtonLeft + (ButtonWidth + ButtonGap) * 3,
                   ButtonTop,
                   ButtonWidth,
                   ButtonHeight,
                   TRUE);
        MoveWindow(State->CloseButton,
                   ButtonLeft + (ButtonWidth + ButtonGap) * 4,
                   ButtonTop,
                   ButtonWidth,
                   ButtonHeight,
                   TRUE);
    }
}

static
LRESULT
CALLBACK
ChildSession_WindowProc(
    _In_ HWND Window,
    _In_ UINT Message,
    _In_ WPARAM WParam,
    _In_ LPARAM LParam)
{
    PCHILD_SESSION_WINDOW_STATE State =
        (PCHILD_SESSION_WINDOW_STATE)GetWindowLongPtrW(Window, GWLP_USERDATA);

    if (Message == WM_NCCREATE)
    {
        State = (PCHILD_SESSION_WINDOW_STATE)((LPCREATESTRUCTW)LParam)->lpCreateParams;
        SetWindowLongPtrW(Window, GWLP_USERDATA, (LONG_PTR)State);
    }
    if (State != NULL)
    {
        switch (Message)
        {
            case CHILD_SESSION_MESSAGE_DISPLAY_UPDATE:
                if ((HRESULT)WParam != S_OK)
                {
                    ChildSession_SetHResult(State, L"Dynamic resolution update", (HRESULT)WParam);
                } else
                {
                    ChildSession_SetOperation(State, L"Dynamic resolution request submitted");
                }
                return 0;
            case WM_GETMINMAXINFO:
                ((PMINMAXINFO)LParam)->ptMinTrackSize.x = MulDiv(1000, GetDpiForWindow(Window), 96);
                ((PMINMAXINFO)LParam)->ptMinTrackSize.y = MulDiv(360, GetDpiForWindow(Window), 96);
                return 0;
            case WM_DPICHANGED:
            {
                const RECT* Rect = (const RECT*)LParam;

                ChildSession_UpdateConsoleFont(Window, State);
                SetWindowPos(Window, NULL, Rect->left, Rect->top,
                             Rect->right - Rect->left, Rect->bottom - Rect->top,
                             SWP_NOZORDER | SWP_NOACTIVATE);
                ChildSession_LayoutWindow(Window, State);
                return 0;
            }
            case CHILD_SESSION_MESSAGE_DESKTOP_SIZE:
            {
                WCHAR Text[128];

                StringCchPrintfW(Text, ARRAYSIZE(Text), L"Remote Desktop size: %lu x %lu", (ULONG)WParam, (ULONG)LParam);
                ChildSession_SetOperation(State, Text);
                return 0;
            }
            case WM_SIZE:
                ChildSession_LayoutWindow(Window, State);
                return 0;
            case WM_SETFOCUS:
                SetFocus(State->ConnectButton);
                return 0;
            case WM_COMMAND:
                if (LOWORD(WParam) >= CHILD_SESSION_BUTTON_CONFIGURATION &&
                    LOWORD(WParam) < CHILD_SESSION_BUTTON_CONFIGURATION + ARRAYSIZE(State->ConfigurationButtons))
                {
                    ChildSession_ChangeConfiguration(Window, State,
                                                     LOWORD(WParam) - CHILD_SESSION_BUTTON_CONFIGURATION);
                    return 0;
                }
                switch (LOWORD(WParam))
                {
                    case CHILD_SESSION_BUTTON_REFRESH:
                    {
                        W32ERROR Error = ChildSession_RefreshWindow(State);

                        if (Error == ERROR_SUCCESS)
                        {
                            ChildSession_SetOperation(State,
                                                      State->RdpWindow.ConnectPending ?
                                                          L"Waiting for logon; status query deferred" :
                                                          L"Status refreshed");
                        } else
                        {
                            ChildSession_SetWin32Error(State, L"Refresh status", Error);
                        }
                        return 0;
                    }
                    case CHILD_SESSION_BUTTON_CONNECT:
                        ChildSession_Connect(Window, State);
                        return 0;
                    case CHILD_SESSION_BUTTON_DISCONNECT:
                        ChildSession_Disconnect(Window, State);
                        return 0;
                    case CHILD_SESSION_BUTTON_LOGOFF:
                        ChildSession_Logoff(Window, State);
                        return 0;
                    case CHILD_SESSION_BUTTON_CLOSE:
                        SendMessageW(Window, WM_CLOSE, 0, 0);
                        return 0;
                }
                break;
            case CHILD_SESSION_MESSAGE_EVENT:
                if (WParam == 1 || WParam == 11 || WParam == 18 || WParam == 19 || WParam == 22)
                {
                    WCHAR Text[256];
                    PCWSTR EventName = WParam == 1 ? L"Connecting" :
                                       WParam == 11 ? L"Warning" :
                                       WParam == 18 ? L"Authentication dialog displayed" :
                                       WParam == 19 ? L"Authentication dialog dismissed" :
                                                       L"Logon event";

                    StringCchPrintfW(Text,
                                     ARRAYSIZE(Text),
                                     L"Remote Desktop: %s (code %ld)",
                                     EventName,
                                     (LONG)LParam);
                    ChildSession_SetOperation(State, Text);
                    printf("%ls\n", Text);
                    return 0;
                }
                if (WParam == 2 || WParam == 3)
                {
                    SetWindowTextW(Window,
                                   WParam == 2 ?
                                       CHILD_SESSION_WINDOW_TITLE L" - Connected" :
                                       CHILD_SESSION_WINDOW_TITLE L" - Active");
                    ChildSession_SetOperation(State,
                                              WParam == 2 ?
                                                  L"Remote Desktop connected; waiting for logon" :
                                                  L"Child session is active");
                    printf(WParam == 2 ?
                                        "Remote Desktop connected\n" :
                                        "Child session is active\n");
                } else
                {
                    WCHAR Text[256];

                    State->DisconnectReason = (LONG)LParam;
                    SetWindowTextW(Window, CHILD_SESSION_WINDOW_TITLE);
                    StringCchPrintfW(Text,
                                     ARRAYSIZE(Text),
                                     WParam == 10 ?
                                         L"Remote Desktop fatal error: %ld" :
                                         L"Remote Desktop disconnected: reason %ld",
                                     State->DisconnectReason);
                    ChildSession_SetOperation(State, Text);
                    printf(WParam == 10 ?
                                        "Remote Desktop fatal error %ld\n" :
                                        "Remote Desktop disconnected with reason %ld\n",
                                    State->DisconnectReason);
                    ChildSession_RdpWindowDestroy(&State->RdpWindow);
                }
                ChildSession_RefreshWindow(State);
                return 0;
            case CHILD_SESSION_MESSAGE_RDP_CLOSE:
                ChildSession_RdpWindowDestroy(&State->RdpWindow);
                ChildSession_SetOperation(State,
                                          L"Remote Desktop window closed and disconnected");
                printf("Remote Desktop window closed and disconnected\n");
                ChildSession_RefreshWindow(State);
                return 0;
            case WM_TIMER:
                ChildSession_RefreshWindow(State);
                return 0;
            case WM_CLOSE:
                DestroyWindow(Window);
                return 0;
            case WM_DESTROY:
                KillTimer(Window, 1);
                PostQuitMessage(0);
                return 0;
        }
    }
    return DefWindowProcW(Window, Message, WParam, LParam);
}

static
HRESULT
ChildSession_RunWindow(
    _In_ PCHILD_SESSION_CONFIGURATION_STATE Configuration)
{
    WNDCLASSEXW ConsoleWindowClass = { sizeof(ConsoleWindowClass) };
    WNDCLASSEXW RdpWindowClass = { sizeof(RdpWindowClass) };
    CHILD_SESSION_WINDOW_STATE WindowState = { 0 };
    HMODULE Instance = GetModuleHandleW(NULL);
    HWND Window = NULL;
    DPI_AWARENESS_CONTEXT PreviousDpiContext;
    MSG Message;
    HRESULT Result;
    W32ERROR Error;
    BOOL MessageResult;
    BOOLEAN Initialized = FALSE;
    BOOLEAN ConsoleClassRegistered = FALSE, RdpClassRegistered = FALSE;

    WindowState.Configuration = Configuration;
    printf("Remote Desktop process PMv2 before initialization: %s\n",
                    AreDpiAwarenessContextsEqual(GetDpiAwarenessContextForProcess(GetCurrentProcess()),
                                                DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) ? "yes" : "no");
    // RDP-created threads must inherit PMv2 as well as the host UI thread.
    if (!AreDpiAwarenessContextsEqual(GetDpiAwarenessContextForProcess(GetCurrentProcess()),
                                     DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) &&
        !SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
    {
        Error = Err_GetLastError();
        printf("Set process PMv2 failed with Win32 error %lu\n", Error);
        return HRESULT_FROM_WIN32(Error);
    }
    PreviousDpiContext = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    if (PreviousDpiContext == NULL) return HRESULT_FROM_WIN32(Err_GetLastError());
    printf("Remote Desktop DPI context: process PMv2=%s, thread PMv2=%s\n",
                    AreDpiAwarenessContextsEqual(GetDpiAwarenessContextForProcess(GetCurrentProcess()),
                                                DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) ? "yes" : "no",
                    AreDpiAwarenessContextsEqual(GetThreadDpiAwarenessContext(),
                                                DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) ? "yes" : "no");
    Result = OleInitialize(NULL);
    if (SUCCEEDED(Result)) Initialized = TRUE;
    if (SUCCEEDED(Result))
    {
        if (!ChildSession_AxHostInitialize())
        {
            Result = E_FAIL;
        } else
        {
            WindowState.AtlAxGetControl = ChildSession_AxHostGetControl;
            printf("Remote Desktop ActiveX host: %ls (Visual Studio ATL)\n",
                            CHILD_SESSION_HOST_CLASS);
        }
    }
    if (SUCCEEDED(Result))
    {
        ConsoleWindowClass.style = CS_HREDRAW | CS_VREDRAW;
        ConsoleWindowClass.lpfnWndProc = ChildSession_WindowProc;
        ConsoleWindowClass.hInstance = Instance;
        ConsoleWindowClass.hCursor = LoadCursorW(NULL, IDC_ARROW);
        ConsoleWindowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        ConsoleWindowClass.lpszClassName = CHILD_SESSION_CONSOLE_WINDOW_CLASS;
        if (RegisterClassExW(&ConsoleWindowClass) == 0)
        {
            Result = HRESULT_FROM_WIN32(Err_GetLastError());
        } else
        {
            ConsoleClassRegistered = TRUE;
        }
    }
    if (SUCCEEDED(Result))
    {
        RdpWindowClass.style = CS_HREDRAW | CS_VREDRAW;
        RdpWindowClass.lpfnWndProc = ChildSession_RdpWindowProc;
        RdpWindowClass.hInstance = Instance;
        RdpWindowClass.hCursor = LoadCursorW(NULL, IDC_ARROW);
        RdpWindowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        RdpWindowClass.lpszClassName = CHILD_SESSION_RDP_WINDOW_CLASS;
        if (RegisterClassExW(&RdpWindowClass) == 0)
        {
            Result = HRESULT_FROM_WIN32(Err_GetLastError());
        } else
        {
            RdpClassRegistered = TRUE;
        }
    }
    if (SUCCEEDED(Result))
    {
        Window = CreateWindowExW(0,
                                 CHILD_SESSION_CONSOLE_WINDOW_CLASS,
                                 CHILD_SESSION_WINDOW_TITLE,
                                 WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 1120,
                                 360,
                                 NULL,
                                 NULL,
                                 Instance,
                                 &WindowState);
        if (Window == NULL) Result = HRESULT_FROM_WIN32(Err_GetLastError());
    }
    if (SUCCEEDED(Result))
    {
        WindowState.ConfigurationText = CreateWindowExW(0,
                                                        L"STATIC",
                                                        L"Configuration: loading...",
                                                        WS_CHILD | WS_VISIBLE | SS_LEFT,
                                                        0, 0, 0, 0,
                                                        Window,
                                                        NULL,
                                                        Instance,
                                                        NULL);
        WindowState.SessionText = CreateWindowExW(0,
                                                  L"STATIC",
                                                  L"Child session: loading...",
                                                  WS_CHILD | WS_VISIBLE | SS_LEFT,
                                                  0, 0, 0, 0,
                                                  Window,
                                                  NULL,
                                                  Instance,
                                                  NULL);
        WindowState.OperationText = CreateWindowExW(0,
                                                    L"STATIC",
                                                    L"Ready",
                                                    WS_CHILD | WS_VISIBLE | SS_LEFT,
                                                    0, 0, 0, 0,
                                                    Window,
                                                    NULL,
                                                    Instance,
                                                    NULL);
        WindowState.ResolutionText = CreateWindowExW(0,
                                                     L"STATIC",
                                                     L"Resolution:",
                                                     WS_CHILD | WS_VISIBLE | SS_LEFT,
                                                     0, 0, 0, 0,
                                                     Window,
                                                     NULL,
                                                     Instance,
                                                     NULL);
        WindowState.ResolutionCombo = CreateWindowExW(0,
                                                      L"COMBOBOX",
                                                      NULL,
                                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                                                          CBS_DROPDOWNLIST | WS_VSCROLL,
                                                      0, 0, 0, 0,
                                                      Window,
                                                      NULL,
                                                      Instance,
                                                      NULL);
        WindowState.RefreshButton = CreateWindowExW(0,
                                                    L"BUTTON",
                                                    L"Refresh",
                                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                                    0, 0, 0, 0,
                                                    Window,
                                                    (HMENU)(ULONG_PTR)CHILD_SESSION_BUTTON_REFRESH,
                                                    Instance,
                                                    NULL);
        WindowState.ConnectButton = CreateWindowExW(0,
                                                    L"BUTTON",
                                                    L"Connect",
                                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                                    0, 0, 0, 0,
                                                    Window,
                                                    (HMENU)(ULONG_PTR)CHILD_SESSION_BUTTON_CONNECT,
                                                    Instance,
                                                    NULL);
        WindowState.DisconnectButton = CreateWindowExW(0,
                                                       L"BUTTON",
                                                       L"Disconnect",
                                                       WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                                       0, 0, 0, 0,
                                                       Window,
                                                       (HMENU)(ULONG_PTR)CHILD_SESSION_BUTTON_DISCONNECT,
                                                       Instance,
                                                       NULL);
        WindowState.LogoffButton = CreateWindowExW(0,
                                                   L"BUTTON",
                                                   L"Log off",
                                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                                   0, 0, 0, 0,
                                                   Window,
                                                   (HMENU)(ULONG_PTR)CHILD_SESSION_BUTTON_LOGOFF,
                                                   Instance,
                                                   NULL);
        WindowState.CloseButton = CreateWindowExW(0,
                                                  L"BUTTON",
                                                  L"Close",
                                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                                  0, 0, 0, 0,
                                                  Window,
                                                  (HMENU)(ULONG_PTR)CHILD_SESSION_BUTTON_CLOSE,
                                                  Instance,
                                                  NULL);
        const PCWSTR ConfigurationLabels[] = {
            L"Enable child sessions", L"Disable child sessions",
            L"Enable localhost credential delegation", L"Remove helper delegation entries",
            L"Enable Hello-only", L"Disable Hello-only (allow passwords)"
        };
        for (ULONG Index = 0; Index < ARRAYSIZE(WindowState.ConfigurationButtons); Index++)
        {
            WindowState.ConfigurationButtons[Index] = CreateWindowExW(
                0, L"BUTTON", ConfigurationLabels[Index], WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                0, 0, 0, 0, Window, (HMENU)(ULONG_PTR)(CHILD_SESSION_BUTTON_CONFIGURATION + Index),
                Instance, NULL);
            if (WindowState.ConfigurationButtons[Index] == NULL)
            {
                Result = HRESULT_FROM_WIN32(Err_GetLastError());
            }
        }
        WindowState.ConfigurationNote = CreateWindowExW(
            0, L"STATIC", L"Configuration buttons save machine-wide changes; closing the demo does not undo them.",
            WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 0, 0, 0, Window, NULL, Instance, NULL);
        if (WindowState.ConfigurationNote == NULL ||
            WindowState.ConfigurationText == NULL || WindowState.SessionText == NULL ||
            WindowState.OperationText == NULL || WindowState.ResolutionText == NULL ||
            WindowState.ResolutionCombo == NULL || WindowState.RefreshButton == NULL ||
            WindowState.ConnectButton == NULL || WindowState.DisconnectButton == NULL ||
            WindowState.LogoffButton == NULL || WindowState.CloseButton == NULL)
        {
            Result = HRESULT_FROM_WIN32(Err_GetLastError());
        }
    }
    if (SUCCEEDED(Result))
    {
        ChildSession_UpdateConsoleFont(Window, &WindowState);
        SetWindowPos(Window, NULL, 0, 0,
                     MulDiv(1120, GetDpiForWindow(Window), 96), MulDiv(360, GetDpiForWindow(Window), 96),
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        for (ULONG Index = 0; Index < ARRAYSIZE(g_ChildSessionResolutions); Index++)
        {
            SendMessageW(WindowState.ResolutionCombo,
                         CB_ADDSTRING,
                         0,
                         (LPARAM)g_ChildSessionResolutions[Index].Name);
        }
        SendMessageW(WindowState.ResolutionCombo, CB_SETCURSEL, 3, 0);
        ChildSession_LayoutWindow(Window, &WindowState);
    }
    if (SUCCEEDED(Result))
    {
        Error = ChildSession_RefreshWindow(&WindowState);
        if (Error != ERROR_SUCCESS)
        {
            ChildSession_SetWin32Error(&WindowState, L"Initial status refresh", Error);
        }
        SetTimer(Window, 1, 1000, NULL);
        ShowWindow(Window, SW_SHOW);
        UpdateWindow(Window);
        SetFocus(WindowState.ConnectButton);
    }
    if (SUCCEEDED(Result))
    {
        while ((MessageResult = GetMessageW(&Message, NULL, 0, 0)) > 0)
        {
            HWND Focus = GetFocus();

            if (WindowState.RdpWindow.ActiveObject != NULL &&
                Message.message >= WM_KEYFIRST && Message.message <= WM_KEYLAST &&
                (Focus == WindowState.RdpWindow.HostWindow ||
                 IsChild(WindowState.RdpWindow.HostWindow, Focus)) &&
                WindowState.RdpWindow.ActiveObject->TranslateAccelerator(&Message) == S_OK)
            {
                continue;
            }
            if ((Message.hwnd != Window && !IsChild(Window, Message.hwnd)) ||
                !IsDialogMessageW(Window, &Message))
            {
                TranslateMessage(&Message);
                DispatchMessageW(&Message);
            }
        }
        if (MessageResult == -1) Result = HRESULT_FROM_WIN32(Err_GetLastError());
    }

    ChildSession_RdpWindowDestroy(&WindowState.RdpWindow);
    if (WindowState.OwnsChildSession)
    {
        Error = ChildSession_LogoffChildSession(TRUE);
        if (Error != ERROR_SUCCESS)
        {
            printf("Child session cleanup failed with Win32 error %lu\n", Error);
            if (SUCCEEDED(Result)) Result = HRESULT_FROM_WIN32(Error);
        } else
        {
            printf("Sample-created child session logged off during cleanup\n");
        }
    }
    if (Window != NULL && IsWindow(Window)) DestroyWindow(Window);
    if (WindowState.Font != NULL) DeleteObject(WindowState.Font);
    if (RdpClassRegistered) UnregisterClassW(CHILD_SESSION_RDP_WINDOW_CLASS, Instance);
    if (ConsoleClassRegistered)
    {
        UnregisterClassW(CHILD_SESSION_CONSOLE_WINDOW_CLASS, Instance);
    }
    if (Initialized) OleUninitialize();
    if (SetThreadDpiAwarenessContext(PreviousDpiContext) == NULL && SUCCEEDED(Result))
    {
        Result = HRESULT_FROM_WIN32(Err_GetLastError());
    }
    return Result;
}

int
_cdecl
wmain(
    _In_ int argc,
    _In_reads_(argc) _Pre_z_ wchar_t** argv)
{
    CHILD_SESSION_CONFIGURATION_STATE Configuration;
    W32ERROR Error, CleanupError;
    HRESULT Result;

    UNREFERENCED_PARAMETER(argv);
    if (argc != 1)
    {
        printf("usage: ChildSession.exe\n");
        return EXIT_FAILURE;
    }
    Error = ChildSession_PrepareConfiguration(&Configuration);
    if (Error != ERROR_SUCCESS)
    {
        printf("Child session preparation failed with Win32 error %lu\n", Error);
        Result = HRESULT_FROM_WIN32(Error);
        goto CleanupConfiguration;
    }
    Result = ChildSession_RunWindow(&Configuration);
    if (FAILED(Result))
    {
        printf("Child session console failed with HRESULT 0x%08lX\n", (ULONG)Result);
    }

CleanupConfiguration:
    CleanupError = ChildSession_RestoreConfiguration(&Configuration);
    if (CleanupError != ERROR_SUCCESS)
    {
        printf("Child session configuration restore failed with Win32 error %lu\n", CleanupError);
        if (SUCCEEDED(Result)) Result = HRESULT_FROM_WIN32(CleanupError);
    }
    return SUCCEEDED(Result) ? EXIT_SUCCESS : EXIT_FAILURE;
}
