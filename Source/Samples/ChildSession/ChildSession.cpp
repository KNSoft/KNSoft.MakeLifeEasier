/*
 * ChildSession: Host the Remote Desktop ActiveX control and connect it to a
 * child session on the local machine.
 *
 * Run: "ChildSession.exe", need administrator privilege.
 */

#define MLE_API
#define _USE_COMMCTL60

#include "../../KNSoft.MakeLifeEasier/MakeLifeEasier.h"

#include "resource.h"

#include <ObjBase.h>
#include <OcIdl.h>
#include <OleAuto.h>
#include <Ole2.h>

#import "libid:8C11EFA1-92C3-11D1-BC1E-00C04FA31489" version("1.0") \
    raw_interfaces_only named_guids rename_namespace("MSTSCLib") \
    exclude("wireHWND", "_RemotableHandle", "__MIDL_IWinTypes_0009")

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "OleAut32.lib")

static const UNICODE_STRING g_ChildSessionCredentialKey =
    RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SOFTWARE\\Policies\\Microsoft\\Windows\\CredentialsDelegation");
static const UNICODE_STRING g_ChildSessionCredentialValue = RTL_CONSTANT_STRING(L"2147483647");
static const UNICODE_STRING g_ChildSessionCredentialTarget = RTL_CONSTANT_STRING(L"TERMSRV/localhost");
static const UNICODE_STRING g_ChildSessionCredentialPolicies[] = {
    RTL_CONSTANT_STRING(L"AllowDefaultCredentials"),
    RTL_CONSTANT_STRING(L"AllowDefCredentialsWhenNTLMOnly")
};
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
    if (ChildSession_SessionIsRegistryValueMissing(Status))
    {
        return STATUS_SUCCESS;
    }
    if (!NT_SUCCESS(Status) || PolicyEnabled != 1)
    {
        return Status;
    }
    Status = Sys_RegOpenKeyEx(&ListKey, Key, KEY_QUERY_VALUE, Policy);
    if (ChildSession_SessionIsRegistryValueMissing(Status))
    {
        return STATUS_SUCCESS;
    }
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    Status = Sys_RegQueryData(ListKey, &g_ChildSessionCredentialValue, &Data);
    NtClose(ListKey);
    if (ChildSession_SessionIsRegistryValueMissing(Status))
    {
        return STATUS_SUCCESS;
    }
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
    HANDLE ListKey;
    NTSTATUS Status;

    if (Enabled)
    {
        Status = Sys_RegSetDword(Key, Policy, 1);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
        Status = Sys_RegCreateKeyEx(&ListKey, Key, KEY_SET_VALUE, Policy, REG_OPTION_NON_VOLATILE, NULL);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
        Status = Sys_RegSetData(ListKey,
                                &g_ChildSessionCredentialValue,
                                REG_SZ,
                                g_ChildSessionCredentialTarget.Buffer,
                                g_ChildSessionCredentialTarget.Length + sizeof(WCHAR));
    } else
    {
        Status = Sys_RegOpenKeyEx(&ListKey, Key, KEY_SET_VALUE, Policy);
        if (ChildSession_SessionIsRegistryValueMissing(Status))
        {
            return STATUS_SUCCESS;
        }
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
        Status = NtDeleteValueKey(ListKey, (PUNICODE_STRING)&g_ChildSessionCredentialValue);
        if (ChildSession_SessionIsRegistryValueMissing(Status))
        {
            Status = STATUS_SUCCESS;
        }
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

    if (!WinStationGetChildSessionId(&Value))
    {
        Error = Err_GetLastError();
        return Error == ERROR_FILE_NOT_FOUND ? ERROR_NOT_FOUND : Error;
    }
    if (Value == MAXULONG)
    {
        return ERROR_NOT_FOUND;
    }
    *SessionId = Value;
    return ERROR_SUCCESS;
}

static
W32ERROR
ChildSession_QueryChildSessionCredentialDelegation(
    _Out_ PBOOLEAN Enabled)
{
    HANDLE Key;
    BOOLEAN PolicyEnabled;
    NTSTATUS Status;

    *Enabled = FALSE;
    Status = Sys_RegOpenKey(&Key,
                            KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                            &g_ChildSessionCredentialKey);
    if (ChildSession_SessionIsRegistryValueMissing(Status))
    {
        return ERROR_SUCCESS;
    }
    if (!NT_SUCCESS(Status))
    {
        return ChildSession_SessionStatusToWin32Error(Status);
    }
    *Enabled = TRUE;
    for (const auto& Policy : g_ChildSessionCredentialPolicies)
    {
        Status = ChildSession_SessionQueryCredentialPolicy(Key, &Policy, &PolicyEnabled);
        if (!NT_SUCCESS(Status) || !PolicyEnabled)
        {
            *Enabled = FALSE;
            break;
        }
    }
    NtClose(Key);
    return ChildSession_SessionStatusToWin32Error(Status);
}

static
W32ERROR
ChildSession_SetChildSessionCredentialDelegation(
    _In_ BOOLEAN Enabled)
{
    HANDLE Key;
    NTSTATUS Status;

    if (Enabled)
    {
        Status = Sys_RegCreateKey(&Key,
                                  KEY_SET_VALUE | KEY_CREATE_SUB_KEY,
                                  &g_ChildSessionCredentialKey);
    } else
    {
        Status = Sys_RegOpenKey(&Key,
                                KEY_ENUMERATE_SUB_KEYS,
                                &g_ChildSessionCredentialKey);
        if (ChildSession_SessionIsRegistryValueMissing(Status))
        {
            return ERROR_SUCCESS;
        }
    }
    if (!NT_SUCCESS(Status))
    {
        return ChildSession_SessionStatusToWin32Error(Status);
    }
    for (const auto& Policy : g_ChildSessionCredentialPolicies)
    {
        Status = ChildSession_SessionSetCredentialPolicy(Key, &Policy, Enabled);
        if (!NT_SUCCESS(Status))
        {
            break;
        }
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
    if (ChildSession_SessionIsRegistryValueMissing(Status))
    {
        return ERROR_SUCCESS;
    }
    if (!NT_SUCCESS(Status))
    {
        return ChildSession_SessionStatusToWin32Error(Status);
    }
    Status = Sys_RegQueryDword(Key, &g_ChildSessionPasswordlessValue, &Value);
    NtClose(Key);
    if (ChildSession_SessionIsRegistryValueMissing(Status))
    {
        return ERROR_SUCCESS;
    }
    if (NT_SUCCESS(Status))
    {
        *Enabled = Value == 2;
    }
    return ChildSession_SessionStatusToWin32Error(Status);
}

static
W32ERROR
ChildSession_SetChildSessionHelloOnly(
    _In_ BOOLEAN Enabled)
{
    HANDLE Key;
    ULONG Value = Enabled ? 2 : 0;
    NTSTATUS Status;

    Status = Sys_RegCreateKey(&Key, KEY_SET_VALUE, &g_ChildSessionPasswordlessKey);
    if (!NT_SUCCESS(Status))
    {
        return ChildSession_SessionStatusToWin32Error(Status);
    }
    Status = Sys_RegSetDword(Key, &g_ChildSessionPasswordlessValue, Value);
    NtClose(Key);
    return ChildSession_SessionStatusToWin32Error(Status);
}

static
W32ERROR
ChildSession_QueryParentInformation(
    _Out_ PWINSTATIONINFORMATION Information)
{
    ULONG ReturnLength;

    if (!WinStationQueryInformationW(WINSTATION_CURRENT_SERVER,
                                     WINSTATION_CURRENT_SESSION,
                                     WinStationInformation,
                                     Information,
                                     sizeof(*Information),
                                     &ReturnLength))
    {
        return Err_GetLastError();
    }
    if (Information->ConnectState != State_Active)
    {
        return ERROR_CTX_WINSTATION_NOT_FOUND;
    }
    return Information->UserName[0] != UNICODE_NULL ? ERROR_SUCCESS : ERROR_NOT_LOGGED_ON;
}

static
W32ERROR
ChildSession_LogoffChildSession(VOID)
{
    ULONG SessionId;
    W32ERROR Error;

    Error = ChildSession_GetChildSessionId(&SessionId);
    if (Error == ERROR_NOT_FOUND)
    {
        return ERROR_SUCCESS;
    }
    if (Error != ERROR_SUCCESS)
    {
        return Error;
    }
    return WinStationReset(WINSTATION_CURRENT_SERVER, SessionId, TRUE) ?
               ERROR_SUCCESS :
               Err_GetLastError();
}

#define CHILD_SESSION_RDP_WINDOW_TITLE L"KNSoft Child Session"

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

typedef struct _CHILD_SESSION_WINDOW_STATE
{
    HWND Window;
    PUI_RDP_CONTEXT Dialog;
    HFONT Font;
    BOOLEAN RestoreEnabled;
} CHILD_SESSION_WINDOW_STATE, *PCHILD_SESSION_WINDOW_STATE;

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
    if (PropertyName == NULL)
    {
        return E_OUTOFMEMORY;
    }
    VariantInit(&Property);
    Property.vt = VT_BOOL;
    Property.boolVal = Value;
    Result = Settings->put_Property(PropertyName, &Property);
    SysFreeString(PropertyName);
    return Result;
}

static
VOID
ChildSession_RefreshWindow(
    _In_ PCHILD_SESSION_WINDOW_STATE State)
{
    BOOLEAN Connecting = State->Dialog != NULL && State->Dialog->State.ConnectPending;

    for (UINT Id = IDC_ENABLED; Id <= IDC_ALLOW_PASSWORD; Id++)
    {
        EnableWindow(GetDlgItem(State->Window, Id), !Connecting);
    }
    EnableWindow(GetDlgItem(State->Window, IDC_CONNECT), State->Dialog == NULL);
    EnableWindow(GetDlgItem(State->Window, IDC_DISCONNECT), State->Dialog != NULL);
    EnableWindow(GetDlgItem(State->Window, IDC_RESOLUTION), State->Dialog == NULL);
}

static
VOID
ChildSession_QueryConfiguration(
    _In_ HWND Window,
    _In_ UINT Id)
{
    BOOLEAN Enabled;
    W32ERROR Error;

    if (Id == IDC_ENABLED)
    {
        Error = WinStationIsChildSessionsEnabled(&Enabled) ? ERROR_SUCCESS : Err_GetLastError();
    } else if (Id == IDC_CREDENTIAL_DELEGATION)
    {
        Error = ChildSession_QueryChildSessionCredentialDelegation(&Enabled);
    } else
    {
        Error = ChildSession_QueryChildSessionHelloOnly(&Enabled);
    }
    if (Error == ERROR_SUCCESS)
    {
        CheckDlgButton(Window,
                       Id,
                       (Id == IDC_ALLOW_PASSWORD ? !Enabled : Enabled) ? BST_CHECKED : BST_UNCHECKED);
    } else
    {
        IO_ConPrintF("Query configuration %u failed: Win32 error %lu\n", Id, Error);
    }
}

static
W32ERROR
ChildSession_RestoreConfiguration(
    _Inout_ PCHILD_SESSION_WINDOW_STATE State)
{
    if (State->RestoreEnabled)
    {
        if (!WinStationEnableChildSessions(FALSE))
        {
            return Err_GetLastError();
        }
        State->RestoreEnabled = FALSE;
    }
    return ERROR_SUCCESS;
}

static
VOID
CALLBACK
ChildSession_RdpEvent(
    _Inout_ PUI_RDP_CONTEXT Data,
    _In_ DISPID Id,
    _In_ DISPPARAMS* Parameters,
    _In_ PVOID Context)
{
    PCHILD_SESSION_WINDOW_STATE State = (PCHILD_SESSION_WINDOW_STATE)Context;

    if (Id == MSTSCAXEVENT_DISPID_REMOTEDESKTOPSIZECHANGE)
    {
        IO_ConPrintF("Remote Desktop size: %lu x %lu\n",
                     Data->State.DesktopWidth,
                     Data->State.DesktopHeight);
    } else if (Id == MSTSCAXEVENT_DISPID_CONNECTED || Id == MSTSCAXEVENT_DISPID_LOGINCOMPLETE)
    {
        IO_ConPrintF(Id == MSTSCAXEVENT_DISPID_CONNECTED ?
                         "Remote Desktop connected; waiting for logon\n" : "Child session is active\n");
    } else if (Id == MSTSCAXEVENT_DISPID_DISCONNECTED || Id == MSTSCAXEVENT_DISPID_FATALERROR)
    {
        IO_ConPrintF(Id == MSTSCAXEVENT_DISPID_FATALERROR ?
                         "Remote Desktop fatal error: %ld\n" :
                         "Remote Desktop disconnected: reason %ld\n",
                     Parameters->rgvarg[0].lVal);
        // The COM callback borrows Data; destroy this window after returning to the message loop.
        PostMessageW(Data->Window, WM_CLOSE, 0, 0);
    } else if (Id == MSTSCAXEVENT_DISPID_CONNECTING || Id == MSTSCAXEVENT_DISPID_WARNING ||
               Id == MSTSCAXEVENT_DISPID_LOGONERROR || Id == MSTSCAXEVENT_DISPID_INTERNALDIALOGDISPLAYED ||
               Id == MSTSCAXEVENT_DISPID_INTERNALDIALOGDISMISSED)
    {
        PCWSTR Name = Id == MSTSCAXEVENT_DISPID_CONNECTING ? L"Connecting" :
                      Id == MSTSCAXEVENT_DISPID_WARNING ? L"Warning" :
                      Id == MSTSCAXEVENT_DISPID_LOGONERROR ? L"Logon event" :
                      Id == MSTSCAXEVENT_DISPID_INTERNALDIALOGDISPLAYED ?
                          L"Authentication dialog displayed" : L"Authentication dialog dismissed";
        LONG Code = Id == MSTSCAXEVENT_DISPID_WARNING || Id == MSTSCAXEVENT_DISPID_LOGONERROR ?
                        Parameters->rgvarg[0].lVal : 0;

        IO_ConPrintF("Remote Desktop: %ls (code %ld)\n", Name, Code);
    } else
    {
        return;
    }
    if (Id == MSTSCAXEVENT_DISPID_CONNECTING || Id == MSTSCAXEVENT_DISPID_CONNECTED ||
        Id == MSTSCAXEVENT_DISPID_LOGINCOMPLETE)
    {
        ChildSession_RefreshWindow(State);
    }
}

static
W32ERROR
ChildSession_Logoff(
    _Inout_ PCHILD_SESSION_WINDOW_STATE State)
{
    W32ERROR Error;

    Error = ChildSession_LogoffChildSession();
    if (Error != ERROR_SUCCESS)
    {
        IO_ConPrintF("Log off child session failed: Win32 error %lu\n", Error);
        return Error;
    }
    Error = ChildSession_RestoreConfiguration(State);
    if (Error != ERROR_SUCCESS)
    {
        IO_ConPrintF("Restore child-session configuration failed: Win32 error %lu\n", Error);
        return Error;
    }
    IO_ConPrintF("Child session logged off\n");
    ChildSession_QueryConfiguration(State->Window, IDC_ENABLED);
    return ERROR_SUCCESS;
}

static
LRESULT
CALLBACK
ChildSession_RdpWindowSubclassProc(
    _In_ HWND Window,
    _In_ UINT Message,
    _In_ WPARAM WParam,
    _In_ LPARAM LParam,
    _In_ UINT_PTR SubclassId,
    _In_ DWORD_PTR ReferenceData)
{
    if (Message == WM_NCDESTROY)
    {
        PCHILD_SESSION_WINDOW_STATE State = (PCHILD_SESSION_WINDOW_STATE)ReferenceData;

        State->Dialog = NULL;
        RemoveWindowSubclass(Window, ChildSession_RdpWindowSubclassProc, SubclassId);
        // The ActiveX client has already been released during WM_DESTROY.
        ChildSession_Logoff(State);
        ChildSession_RefreshWindow(State);
    }
    return DefSubclassProc(Window, Message, WParam, LParam);
}

static
VOID
ChildSession_RdpWindowDestroy(
    _Inout_ PCHILD_SESSION_WINDOW_STATE State)
{
    if (State->Dialog != NULL)
    {
        DestroyWindow(State->Dialog->Window);
    }
}

static
_Success_(return >= 0)
_At_(State->Dialog, _Post_notnull_)
HRESULT
ChildSession_RdpWindowCreate(
    _In_ HWND OwnerWindow,
    _In_ const CHILD_SESSION_RESOLUTION* Resolution,
    _In_ const WINSTATIONINFORMATION* Information,
    _Inout_ PCHILD_SESSION_WINDOW_STATE State)
{
    MSTSCLib::IMsRdpClient9* Client;
    MSTSCLib::IMsRdpClientAdvancedSettings8* Settings = NULL;
    UI_RDP_OPTIONS Options = { 0 };
    BSTR Server = NULL, UserName = NULL, Domain = NULL;
    MSTSCLib::IMsRdpExtendedSettings* ExtendedSettings = NULL;
    RECT WindowRect;
    MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
    LONG WindowWidth, WindowHeight;
    HRESULT Result;

    SetRect(&WindowRect, 0, 0, (LONG)Resolution->Width, (LONG)Resolution->Height);
    if (!AdjustWindowRectExForDpi(&WindowRect,
                                  WS_OVERLAPPEDWINDOW,
                                  FALSE,
                                  0,
                                  GetDpiForWindow(OwnerWindow)) ||
        !GetMonitorInfoW(MonitorFromWindow(OwnerWindow, MONITOR_DEFAULTTONEAREST), &MonitorInfo))
    {
        return HRESULT_FROM_WIN32(Err_GetLastError());
    }
    WindowWidth = min(WindowRect.right - WindowRect.left,
                      MonitorInfo.rcWork.right - MonitorInfo.rcWork.left);
    WindowHeight = min(WindowRect.bottom - WindowRect.top,
                       MonitorInfo.rcWork.bottom - MonitorInfo.rcWork.top);
    Options.Parent = OwnerWindow;
    Options.Title = CHILD_SESSION_RDP_WINDOW_TITLE;
    Options.Style = WS_OVERLAPPEDWINDOW;
    Options.Rect.left =
        MonitorInfo.rcWork.left + (MonitorInfo.rcWork.right - MonitorInfo.rcWork.left - WindowWidth) / 2;
    Options.Rect.top = MonitorInfo.rcWork.top + (MonitorInfo.rcWork.bottom - MonitorInfo.rcWork.top - WindowHeight) / 2;
    Options.Rect.right = Options.Rect.left + WindowWidth;
    Options.Rect.bottom = Options.Rect.top + WindowHeight;
    Options.Callback = ChildSession_RdpEvent;
    Options.Context = State;
    Result = UI_CreateRdpDialog(&Options, &State->Dialog);
    if (FAILED(Result))
    {
        return Result;
    }
    Client = State->Dialog->Client;
    if (FAILED(Result = Client->QueryInterface(IID_PPV_ARGS(&ExtendedSettings))) ||
        FAILED(Result = ChildSession_SetExtendedBoolean(ExtendedSettings, L"ConnectToChildSession", VARIANT_TRUE)) ||
        FAILED(Result = ChildSession_SetExtendedBoolean(ExtendedSettings,
                                                        L"EnableFrameBufferRedirection",
                                                        VARIANT_TRUE)))
    {
        goto Cleanup;
    }
    // Use the parent session's user, including when run as another administrator.
    Server = SysAllocString(L"localhost");
    UserName = SysAllocString(Information->UserName);
    Domain = SysAllocString(Information->Domain);
    if (Server == NULL || UserName == NULL || Domain == NULL)
    {
        Result = E_OUTOFMEMORY;
        goto Cleanup;
    }
    if (FAILED(Result = Client->put_Server(Server)) ||
        FAILED(Result = Client->put_UserName(UserName)) ||
        FAILED(Result = Client->put_Domain(Domain)) ||
        FAILED(Result = Client->put_DesktopWidth((LONG)Resolution->Width)) ||
        FAILED(Result = Client->put_DesktopHeight((LONG)Resolution->Height)) ||
        FAILED(Result = Client->get_AdvancedSettings9(&Settings)) ||
        FAILED(Result = Settings->put_EnableCredSspSupport(VARIANT_TRUE)) ||
        FAILED(Result = Settings->put_AuthenticationLevel(MSTSCAX_AUTHENTICATION_LEVEL_NONE)))
    {
        goto Cleanup;
    }
    // Keep the selected desktop resolution without stretching the image.
    Result = Settings->put_SmartSizing(VARIANT_FALSE);
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    if (!SetWindowSubclass(State->Dialog->Window, ChildSession_RdpWindowSubclassProc, 0, (DWORD_PTR)State))
    {
        Result = E_FAIL;
        goto Cleanup;
    }
    UI_RdpDialogSetAutoResize(State->Dialog, TRUE);
    ShowWindow(State->Dialog->Window, SW_SHOW);
    UpdateWindow(State->Dialog->Window);
    Result = S_OK;

Cleanup:
    SysFreeString(Domain);
    SysFreeString(UserName);
    SysFreeString(Server);
    if (Settings != NULL)
    {
        Settings->Release();
    }
    if (ExtendedSettings != NULL)
    {
        ExtendedSettings->Release();
    }
    if (FAILED(Result))
    {
        DestroyWindow(State->Dialog->Window);
        State->Dialog = NULL;
    }
    return Result;
}

static
const CHILD_SESSION_RESOLUTION*
ChildSession_GetSelectedResolution(
    _In_ PCHILD_SESSION_WINDOW_STATE State)
{
    LRESULT Index = SendMessageW(GetDlgItem(State->Window, IDC_RESOLUTION), CB_GETCURSEL, 0, 0);

    if (Index < 0 || Index >= ARRAYSIZE(g_ChildSessionResolutions))
    {
        Index = 3;
    }
    return &g_ChildSessionResolutions[Index];
}

static
HRESULT
ChildSession_Connect(
    _In_ HWND Window,
    _Inout_ PCHILD_SESSION_WINDOW_STATE State)
{
    BOOLEAN Enabled;
    WINSTATIONINFORMATION Information;
    const CHILD_SESSION_RESOLUTION* Resolution;
    ULONG SessionId;
    W32ERROR Error;
    HRESULT Result;

    if (State->Dialog != NULL)
    {
        return S_FALSE;
    }
    Error = ChildSession_QueryParentInformation(&Information);
    if (Error != ERROR_SUCCESS)
    {
        IO_ConPrintF("Validate parent session failed: Win32 error %lu\n", Error);
        return HRESULT_FROM_WIN32(Error);
    }
    Error = WinStationIsChildSessionsEnabled(&Enabled) ? ERROR_SUCCESS : Err_GetLastError();
    if (Error != ERROR_SUCCESS)
    {
        IO_ConPrintF("Query configuration failed: Win32 error %lu\n", Error);
        return HRESULT_FROM_WIN32(Error);
    }
    if (!Enabled)
    {
        Error = (WinStationEnableChildSessions(TRUE) ? ERROR_SUCCESS : Err_GetLastError());
        if (Error != ERROR_SUCCESS)
        {
            IO_ConPrintF("Enable child sessions failed: Win32 error %lu\n", Error);
            return HRESULT_FROM_WIN32(Error);
        }
        State->RestoreEnabled = TRUE;
        ChildSession_QueryConfiguration(Window, IDC_ENABLED);
    }

    Error = ChildSession_GetChildSessionId(&SessionId);
    if (Error == ERROR_SUCCESS)
    {
        IO_ConPrintF("Connecting to existing child session ID %lu\n", SessionId);
    } else if (Error == ERROR_NOT_FOUND)
    {
        IO_ConPrintF("Creating and connecting to a child session...\n");
    } else
    {
        IO_ConPrintF("Query child session failed: Win32 error %lu\n", Error);
        Result = HRESULT_FROM_WIN32(Error);
        goto Cleanup;
    }

    Resolution = ChildSession_GetSelectedResolution(State);
    Result = ChildSession_RdpWindowCreate(Window, Resolution, &Information, State);
    if (FAILED(Result))
    {
        IO_ConPrintF("Create Remote Desktop window failed: HRESULT 0x%08lX\n", (ULONG)Result);
        goto Cleanup;
    }
    Result = UI_RdpDialogConnect(State->Dialog);
    if (FAILED(Result))
    {
        IO_ConPrintF("Remote Desktop connect failed: HRESULT 0x%08lX\n", (ULONG)Result);
        goto Cleanup;
    }
    IO_ConPrintF("Remote Desktop resolution: %lu x %lu\n",
                 Resolution->Width,
                 Resolution->Height);
    ChildSession_RefreshWindow(State);
    return S_OK;

Cleanup:
    ChildSession_RdpWindowDestroy(State);
    Error = ChildSession_RestoreConfiguration(State);
    if (Error != ERROR_SUCCESS)
    {
        IO_ConPrintF("Restore child-session configuration failed: Win32 error %lu\n", Error);
    }
    ChildSession_QueryConfiguration(Window, IDC_ENABLED);
    return Result;
}

static
VOID
ChildSession_ChangeConfiguration(
    _Inout_ PCHILD_SESSION_WINDOW_STATE State,
    _In_ UINT Id)
{
    W32ERROR Error;
    BOOLEAN Enabled = IsDlgButtonChecked(State->Window, Id) != BST_CHECKED;

    if (Id == IDC_ENABLED)
    {
        Error = WinStationEnableChildSessions(Enabled) ? ERROR_SUCCESS : Err_GetLastError();
        if (Error == ERROR_SUCCESS)
        {
            // Explicit configuration supersedes the temporary connection-time change.
            State->RestoreEnabled = FALSE;
        }
    } else if (Id == IDC_CREDENTIAL_DELEGATION)
    {
        Error = ChildSession_SetChildSessionCredentialDelegation(Enabled);
    } else
    {
        Error = ChildSession_SetChildSessionHelloOnly(!Enabled);
    }
    ChildSession_QueryConfiguration(State->Window, Id);
    if (Error != ERROR_SUCCESS)
    {
        IO_ConPrintF("Change configuration %u failed: Win32 error %lu\n", Id, Error);
    } else
    {
        IO_ConPrintF("Configuration %u saved; reconnect if necessary\n", Id);
    }
}

static
INT_PTR
CALLBACK
ChildSession_DialogProc(
    _In_ HWND Window,
    _In_ UINT Message,
    _In_ WPARAM WParam,
    _In_ LPARAM LParam)
{
    PCHILD_SESSION_WINDOW_STATE State = (PCHILD_SESSION_WINDOW_STATE)GetWindowLongPtrW(Window, DWLP_USER);

    if (Message == WM_INITDIALOG)
    {
        State = (PCHILD_SESSION_WINDOW_STATE)LParam;
        State->Window = Window;
        SetWindowLongPtrW(Window, DWLP_USER, LParam);
        if (UI_CreateDefaultFont(&State->Font, 0) == ERROR_SUCCESS)
        {
            for (HWND Control = GetWindow(Window, GW_CHILD); Control != NULL; Control = GetWindow(Control, GW_HWNDNEXT))
            {
                UI_SetWindowFont(Control, State->Font, FALSE);
            }
        }
        for (const auto& Resolution : g_ChildSessionResolutions)
        {
            SendDlgItemMessageW(Window, IDC_RESOLUTION, CB_ADDSTRING, 0, (LPARAM)Resolution.Name);
        }
        SendDlgItemMessageW(Window, IDC_RESOLUTION, CB_SETCURSEL, 3, 0);
        for (UINT Id = IDC_ENABLED; Id <= IDC_ALLOW_PASSWORD; Id++)
        {
            ChildSession_QueryConfiguration(Window, Id);
        }
        ChildSession_RefreshWindow(State);
        SetFocus(GetDlgItem(Window, IDC_CONNECT));
        return FALSE;
    }
    if (State == NULL)
    {
        return FALSE;
    }
    if (Message == WM_COMMAND)
    {
        UINT Id = LOWORD(WParam);

        if (Id >= IDC_ENABLED && Id <= IDC_ALLOW_PASSWORD && HIWORD(WParam) == BN_CLICKED)
        {
            ChildSession_ChangeConfiguration(State, Id);
        } else if (Id == IDC_CONNECT)
        {
            ChildSession_Connect(Window, State);
        } else if (Id == IDC_DISCONNECT)
        {
            ChildSession_RdpWindowDestroy(State);
        } else if (Id == IDCANCEL)
        {
            DestroyWindow(Window);
        } else
        {
            return FALSE;
        }
        return TRUE;
    } else if (Message == WM_CLOSE)
    {
        DestroyWindow(Window);
        return TRUE;
    } else if (Message == WM_DESTROY)
    {
        PostQuitMessage(0);
        return TRUE;
    }
    return FALSE;
}

static
HRESULT
ChildSession_RunWindow(
    _Inout_ PCHILD_SESSION_WINDOW_STATE State)
{
    MSG Message;
    HRESULT Result;
    W32ERROR Error;
    BOOL MessageResult;

    Result = OleInitialize(NULL);
    if (FAILED(Result))
    {
        return Result;
    }
    if (CreateDialogParamW((HINSTANCE)&__ImageBase,
                           MAKEINTRESOURCEW(IDD_CHILD_SESSION),
                           NULL,
                           ChildSession_DialogProc,
                           (LPARAM)State) == NULL)
    {
        Result = HRESULT_FROM_WIN32(Err_GetLastError());
        OleUninitialize();
        return Result;
    }
    IO_ConPrintF("Configuration changes are saved immediately and remain after closing this sample.\n");
    ShowWindow(State->Window, SW_SHOW);
    UpdateWindow(State->Window);
    while ((MessageResult = GetMessageW(&Message, NULL, 0, 0)) > 0)
    {
        if (State->Dialog != NULL && UI_RdpDialogTranslateMessage(State->Dialog, &Message))
        {
            continue;
        }
        if ((Message.hwnd != State->Window && !IsChild(State->Window, Message.hwnd)) ||
            !IsDialogMessageW(State->Window, &Message))
        {
            TranslateMessage(&Message);
            DispatchMessageW(&Message);
        }
    }
    if (MessageResult == -1)
    {
        Result = HRESULT_FROM_WIN32(Err_GetLastError());
    }

    ChildSession_RdpWindowDestroy(State);
    Error = ChildSession_LogoffChildSession();
    if (Error != ERROR_SUCCESS)
    {
        IO_ConPrintF("Child session cleanup failed with Win32 error %lu\n", Error);
        if (SUCCEEDED(Result))
        {
            Result = HRESULT_FROM_WIN32(Error);
        }
    }
    if (State->Window != NULL && IsWindow(State->Window))
    {
        DestroyWindow(State->Window);
    }
    if (State->Font != NULL)
    {
        DeleteObject(State->Font);
    }
    OleUninitialize();
    return Result;
}

int
_cdecl
wmain(
    _In_ int argc,
    _In_reads_(argc) _Pre_z_ wchar_t** argv)
{
    CHILD_SESSION_WINDOW_STATE State = { 0 };
    W32ERROR Error;
    HRESULT Result;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    Result = ChildSession_RunWindow(&State);
    if (FAILED(Result))
    {
        IO_ConPrintF("Child session console failed with HRESULT 0x%08lX\n", (ULONG)Result);
    }
    Error = ChildSession_RestoreConfiguration(&State);
    if (Error != ERROR_SUCCESS)
    {
        IO_ConPrintF("Child session configuration restore failed with Win32 error %lu\n", Error);
        if (SUCCEEDED(Result))
        {
            Result = HRESULT_FROM_WIN32(Error);
        }
    }
    return Result;
}
