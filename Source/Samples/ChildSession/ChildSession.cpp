/*
 * ChildSession: Host the Remote Desktop ActiveX control and connect it to a
 * child session on the local machine.
 *
 * Run: "ChildSession.exe", need administrator privilege.
 */

#define MLE_API
#define _USE_COMMCTL60

#include "../../KNSoft.MakeLifeEasier/MakeLifeEasier.h"

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

    *SessionId = MAXULONG;
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
ChildSession_QueryChildSessionInformation(
    _Out_ PWINSTATIONINFORMATION Information)
{
    ULONG SessionId, ReturnLength;
    W32ERROR Error;

    Error = ChildSession_GetChildSessionId(&SessionId);
    if (Error != ERROR_SUCCESS)
    {
        return Error;
    }
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
    if (ChildSession_SessionIsRegistryValueMissing(Status))
    {
        return ERROR_SUCCESS;
    }
    if (!NT_SUCCESS(Status))
    {
        return ChildSession_SessionStatusToWin32Error(Status);
    }
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
    if (NT_SUCCESS(Status))
    {
        *Enabled = DefaultEnabled && NtlmEnabled;
    }
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
    if (Error != ERROR_SUCCESS)
    {
        return Error;
    }
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
    if (Status != STATUS_SUCCESS)
    {
        return Err_NtStatusToWin32Error(Status);
    }
    if (!WinStationQueryInformationW(WINSTATION_CURRENT_SERVER,
                                     WINSTATION_CURRENT_SESSION,
                                     WinStationInformation,
                                     &Information,
                                     sizeof(Information),
                                     &ReturnLength))
    {
        return Err_GetLastError();
    }
    if (Information.ConnectState != State_Active)
    {
        return ERROR_CTX_WINSTATION_NOT_FOUND;
    }
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
    if (Error == ERROR_NOT_FOUND)
    {
        return ERROR_SUCCESS;
    }
    if (Error != ERROR_SUCCESS)
    {
        return Error;
    }
    // TRUE waits for reset/disconnect completion; FALSE only submits the request.
    return WinStationReset(WINSTATION_CURRENT_SERVER, SessionId, Wait) ?
               ERROR_SUCCESS :
               Err_GetLastError();
}


#define CHILD_SESSION_CONSOLE_WINDOW_CLASS L"KNSoft.MakeLifeEasier.ChildSession.Console"
#define CHILD_SESSION_WINDOW_TITLE L"KNSoft Child Session Console"
#define CHILD_SESSION_RDP_WINDOW_TITLE L"KNSoft Child Session"
#define CHILD_SESSION_MESSAGE_EVENT (WM_APP + 1)
#define CHILD_SESSION_MESSAGE_RDP_CLOSE (WM_APP + 2)
#define CHILD_SESSION_MESSAGE_DESKTOP_SIZE (WM_APP + 3)

#define CHILD_SESSION_BUTTON_REFRESH 1001
#define CHILD_SESSION_BUTTON_CONNECT 1002
#define CHILD_SESSION_BUTTON_DISCONNECT 1003
#define CHILD_SESSION_BUTTON_LOGOFF 1004
#define CHILD_SESSION_BUTTON_CLOSE 1005
#define CHILD_SESSION_BUTTON_CONFIGURATION 1010

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

typedef enum _CHILD_SESSION_SETTING
{
    ChildSessionSettingEnabled,
    ChildSessionSettingCredentialDelegation,
    ChildSessionSettingHelloOnly
} CHILD_SESSION_SETTING;

static const struct
{
    PCWSTR Text;
    CHILD_SESSION_SETTING Setting;
    BOOLEAN Enabled;
} g_ChildSessionConfigurationButtons[] = {
    { L"Enable child sessions", ChildSessionSettingEnabled, TRUE },
    { L"Disable child sessions", ChildSessionSettingEnabled, FALSE },
    { L"Enable localhost credential delegation", ChildSessionSettingCredentialDelegation, TRUE },
    { L"Remove helper delegation entries", ChildSessionSettingCredentialDelegation, FALSE },
    { L"Enable Hello-only", ChildSessionSettingHelloOnly, TRUE },
    { L"Disable Hello-only (allow passwords)", ChildSessionSettingHelloOnly, FALSE }
};

typedef struct _CHILD_SESSION_CONFIGURATION_STATE
{
    CHILD_SESSION_CONFIGURATION Original;
    BOOLEAN Valid;
    BOOLEAN EnabledTouched;
} CHILD_SESSION_CONFIGURATION_STATE, *PCHILD_SESSION_CONFIGURATION_STATE;

typedef struct _CHILD_SESSION_RDP_WINDOW_STATE
{
    PUI_RDP_CONTEXT Dialog;
    HWND OwnerWindow;
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
    HWND ConfigurationButtons[ARRAYSIZE(g_ChildSessionConfigurationButtons)];
    HWND ConfigurationNote;
    HFONT Font;
    CHILD_SESSION_RDP_WINDOW_STATE RdpWindow;
    PCHILD_SESSION_CONFIGURATION_STATE Configuration;
    LONG DisconnectReason;
    BOOLEAN OwnsChildSession;
    BOOLEAN SessionExists;
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
ChildSession_ReportOperation(
    _In_ PCHILD_SESSION_WINDOW_STATE State,
    _In_ _Printf_format_string_ PCWSTR Format,
    ...)
{
    WCHAR Text[256];
    va_list Arguments;

    va_start(Arguments, Format);
    Str_VPrintfW(Text, Format, Arguments);
    va_end(Arguments);
    ChildSession_SetOperation(State, Text);
    IO_ConPrintF("%ls\n", Text);
}

static
VOID
ChildSession_SetWin32Error(
    _In_ PCHILD_SESSION_WINDOW_STATE State,
    _In_ PCWSTR Operation,
    _In_ W32ERROR Error)
{
    ChildSession_ReportOperation(State, L"%s failed: Win32 error %lu", Operation, Error);
}

static
VOID
ChildSession_SetHResult(
    _In_ PCHILD_SESSION_WINDOW_STATE State,
    _In_ PCWSTR Operation,
    _In_ HRESULT Result)
{
    ChildSession_ReportOperation(State, L"%s failed: HRESULT 0x%08lX", Operation, (ULONG)Result);
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
        Str_PrintfW(Text,
                    L"Configuration  |  Child sessions: %s%s  |  Credential delegation: %s  |  Hello-only: %s",
                    Configuration.Enabled ? L"Enabled" : L"Disabled",
                    State->Configuration->EnabledTouched ? L" (temporary)" : L"",
                    Configuration.CredentialDelegation ? L"Enabled" : L"Disabled",
                    Configuration.HelloOnly ? L"Enabled" : L"Disabled");
    } else
    {
        Str_PrintfW(Text,
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
            Str_PrintfW(Text,
                        Information.Domain[0] != UNICODE_NULL ?
                            L"Child session  |  ID: %lu  |  State: %s  |  User: %s\\%s" :
                            L"Child session  |  ID: %lu  |  State: %s  |  User: %s%s",
                        Information.LogonId,
                        ChildSession_GetStateName(Information.ConnectState),
                        Information.Domain,
                        Information.UserName);
        } else
        {
            Str_PrintfW(Text,
                        L"Child session  |  ID: %lu  |  State: %s",
                        Information.LogonId,
                        ChildSession_GetStateName(Information.ConnectState));
        }
    } else if (SessionError == ERROR_NOT_FOUND)
    {
        State->SessionExists = FALSE;
        Str_CopyW(Text,
                  L"Child session  |  None - select a resolution and click Create / Connect");
    } else
    {
        State->SessionExists = FALSE;
        Str_PrintfW(Text,
                    L"Child session unavailable: Win32 error %lu",
                    SessionError);
    }
    SetWindowTextW(State->SessionText, Text);
    SetWindowTextW(State->ConnectButton,
                   SessionError == ERROR_NOT_FOUND ? L"Create / Connect" : L"Connect");
    EnableWindow(State->ConnectButton,
                 !State->RdpWindow.Connected && !State->RdpWindow.ConnectPending);
    EnableWindow(State->DisconnectButton,
                 State->RdpWindow.Dialog != NULL);
    EnableWindow(State->LogoffButton, State->SessionExists);
    EnableWindow(State->ResolutionCombo, State->RdpWindow.Dialog == NULL);

    if (ConfigurationError != ERROR_SUCCESS)
    {
        return ConfigurationError;
    }
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
    if (Error != ERROR_SUCCESS)
    {
        return Error;
    }
    Error = ChildSession_QueryChildSessionConfiguration(&State->Original);
    if (Error != ERROR_SUCCESS)
    {
        return Error;
    }
    State->Valid = TRUE;
    return ERROR_SUCCESS;
}

static
W32ERROR
ChildSession_RestoreConfiguration(
    _In_ PCHILD_SESSION_CONFIGURATION_STATE State)
{
    W32ERROR Error, FirstError = ERROR_SUCCESS;

    if (!State->Valid)
    {
        return ERROR_SUCCESS;
    }
    if (State->EnabledTouched)
    {
        Error = (WinStationEnableChildSessions(State->Original.Enabled) ? ERROR_SUCCESS : Err_GetLastError());
        if (Error != ERROR_SUCCESS)
        {
            FirstError = Error;
        }
    }
    return FirstError;
}

static
VOID
CALLBACK
ChildSession_RdpEvent(
    _Inout_ PUI_RDP_CONTEXT Data,
    _In_ DISPID Id,
    _In_ DISPPARAMS* Parameters,
    _In_opt_ PVOID Context)
{
    PCHILD_SESSION_RDP_WINDOW_STATE State = (PCHILD_SESSION_RDP_WINDOW_STATE)Context;
    LONG Error = 0;

    switch (Id)
    {
        case MSTSCAXEVENT_DISPID_REMOTEDESKTOPSIZECHANGE:
            PostMessageW(State->OwnerWindow,
                         CHILD_SESSION_MESSAGE_DESKTOP_SIZE,
                         Parameters->rgvarg[1].lVal,
                         Parameters->rgvarg[0].lVal);
            return;
        case MSTSCAXEVENT_DISPID_DISCONNECTED:
        case MSTSCAXEVENT_DISPID_FATALERROR:
        case MSTSCAXEVENT_DISPID_WARNING:
        case MSTSCAXEVENT_DISPID_LOGONERROR:
            Error = Parameters->rgvarg[0].lVal;
            break;
        case MSTSCAXEVENT_DISPID_CONNECTING:
        case MSTSCAXEVENT_DISPID_CONNECTED:
        case MSTSCAXEVENT_DISPID_LOGINCOMPLETE:
        case MSTSCAXEVENT_DISPID_INTERNALDIALOGDISPLAYED:
        case MSTSCAXEVENT_DISPID_INTERNALDIALOGDISMISSED:
            break;
        default:
            return;
    }
    State->Connected = Data->State.Connected;
    State->ConnectPending = Data->State.ConnectPending;
    PostMessageW(State->OwnerWindow, CHILD_SESSION_MESSAGE_EVENT, Id, Error);
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
        PCHILD_SESSION_RDP_WINDOW_STATE State = (PCHILD_SESSION_RDP_WINDOW_STATE)ReferenceData;

        State->Dialog = NULL;
        State->Connected = FALSE;
        State->ConnectPending = FALSE;
        RemoveWindowSubclass(Window, ChildSession_RdpWindowSubclassProc, SubclassId);
        PostMessageW(State->OwnerWindow, CHILD_SESSION_MESSAGE_RDP_CLOSE, 0, 0);
    }
    return DefSubclassProc(Window, Message, WParam, LParam);
}

static
VOID
ChildSession_RdpWindowDestroy(
    _Inout_ PCHILD_SESSION_RDP_WINDOW_STATE State)
{
    if (State->Dialog != NULL)
    {
        DestroyWindow(State->Dialog->Window);
    }
}

static
HRESULT
ChildSession_RdpWindowCreate(
    _In_ HWND OwnerWindow,
    _In_ const CHILD_SESSION_RESOLUTION* Resolution,
    _Inout_ PCHILD_SESSION_RDP_WINDOW_STATE State)
{
    MSTSCLib::IMsRdpClient9* Client;
    MSTSCLib::IMsRdpClientAdvancedSettings8* Settings = NULL;
    UI_RDP_OPTIONS Options = { 0 };
    BSTR Server = NULL, UserName = NULL, Domain = NULL;
    WINSTATIONINFORMATION Information;
    ULONG ReturnLength;
    MSTSCLib::IMsRdpExtendedSettings* ExtendedSettings = NULL;
    RECT WindowRect;
    MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
    LONG WindowWidth, WindowHeight;
    HRESULT Result;

    State->OwnerWindow = OwnerWindow;
    SetRect(&WindowRect, 0, 0, (LONG)Resolution->Width, (LONG)Resolution->Height);
    if (!AdjustWindowRectEx(&WindowRect, WS_OVERLAPPEDWINDOW, FALSE, 0))
    {
        return HRESULT_FROM_WIN32(Err_GetLastError());
    }
    Options.Parent = OwnerWindow;
    Options.Title = CHILD_SESSION_RDP_WINDOW_TITLE;
    Options.Style = WS_OVERLAPPEDWINDOW;
    SetRect(&Options.Rect, 0, 0, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top);
    Options.Callback = ChildSession_RdpEvent;
    Options.Context = State;
    Result = UI_CreateRdpDialog(&Options, &State->Dialog);
    if (FAILED(Result))
    {
        return Result;
    }
    if (!SetWindowSubclass(State->Dialog->Window, ChildSession_RdpWindowSubclassProc, 0, (DWORD_PTR)State))
    {
        DestroyWindow(State->Dialog->Window);
        State->Dialog = NULL;
        return E_FAIL;
    }
    SetRect(&WindowRect, 0, 0, (LONG)Resolution->Width, (LONG)Resolution->Height);
    if (!AdjustWindowRectExForDpi(&WindowRect,
                                  WS_OVERLAPPEDWINDOW,
                                  FALSE,
                                  0,
                                  GetDpiForWindow(State->Dialog->Window)) ||
        !GetMonitorInfoW(MonitorFromWindow(State->Dialog->Window, MONITOR_DEFAULTTONEAREST), &MonitorInfo))
    {
        Result = HRESULT_FROM_WIN32(Err_GetLastError());
        goto Cleanup;
    }
    WindowWidth = min(WindowRect.right - WindowRect.left,
                      MonitorInfo.rcWork.right - MonitorInfo.rcWork.left);
    WindowHeight = min(WindowRect.bottom - WindowRect.top,
                       MonitorInfo.rcWork.bottom - MonitorInfo.rcWork.top);
    if (!SetWindowPos(State->Dialog->Window,
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
    Client = State->Dialog->Client;
    Result = Client->QueryInterface(IID_PPV_ARGS(&ExtendedSettings));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = ChildSession_SetExtendedBoolean(ExtendedSettings, L"ConnectToChildSession", VARIANT_TRUE);
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = ChildSession_SetExtendedBoolean(ExtendedSettings, L"EnableFrameBufferRedirection", VARIANT_TRUE);
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    // Use the parent session's user, including when the demo was run as another administrator.
    if (!WinStationQueryInformationW(WINSTATION_CURRENT_SERVER,
                                    WINSTATION_CURRENT_SESSION,
                                    WinStationInformation,
                                    &Information,
                                    sizeof(Information),
                                    &ReturnLength))
    {
        Result = HRESULT_FROM_WIN32(Err_GetLastError());
        goto Cleanup;
    }
    if (Information.UserName[0] == UNICODE_NULL)
    {
        Result = HRESULT_FROM_WIN32(ERROR_NOT_LOGGED_ON);
        goto Cleanup;
    }
    Server = SysAllocString(L"localhost");
    UserName = SysAllocString(Information.UserName);
    Domain = SysAllocString(Information.Domain);
    if (Server == NULL || UserName == NULL || Domain == NULL)
    {
        Result = E_OUTOFMEMORY;
        goto Cleanup;
    }
    Result = Client->put_Server(Server);
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Client->put_UserName(UserName);
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Client->put_Domain(Domain);
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Client->put_DesktopWidth((LONG)Resolution->Width);
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Client->put_DesktopHeight((LONG)Resolution->Height);
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Client->get_AdvancedSettings9(&Settings);
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Settings->put_EnableCredSspSupport(VARIANT_TRUE);
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Settings->put_AuthenticationLevel(MSTSCAX_AUTHENTICATION_LEVEL_NONE);
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    // Keep the selected desktop resolution without stretching the image.
    Result = Settings->put_SmartSizing(VARIANT_FALSE);
    if (FAILED(Result))
    {
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
        ChildSession_RdpWindowDestroy(State);
    }
    return Result;
}

static
HRESULT
ChildSession_RdpWindowConnect(
    _Inout_ PCHILD_SESSION_RDP_WINDOW_STATE State)
{
    HRESULT Result;

    State->ConnectPending = TRUE;
    IO_ConPrintF("Remote Desktop Connect: entering\n");
    Result = UI_RdpDialogConnect(State->Dialog);
    IO_ConPrintF("Remote Desktop Connect: returned 0x%08lX\n", (ULONG)Result);
    if (FAILED(Result))
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
    _In_ PCHILD_SESSION_WINDOW_STATE State)
{
    CHILD_SESSION_CONFIGURATION Configuration;
    WINSTATIONINFORMATION Information;
    const CHILD_SESSION_RESOLUTION* Resolution;
    W32ERROR Error;
    HRESULT Result;
    BOOLEAN CreateChildSession = FALSE;

    if (State->RdpWindow.Connected || State->RdpWindow.ConnectPending)
    {
        return S_FALSE;
    }
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
        IO_ConPrintF("Connecting to existing child session ID %lu, state %ls\n",
                        Information.LogonId,
                        ChildSession_GetStateName(Information.ConnectState));
        ChildSession_SetOperation(State, L"Connecting to the existing child session...");
    } else if (Error == ERROR_NOT_FOUND)
    {
        CreateChildSession = TRUE;
        IO_ConPrintF("Creating and connecting to a child session\n");
        ChildSession_SetOperation(State, L"Creating and connecting to a child session...");
    } else
    {
        ChildSession_SetWin32Error(State, L"Query child session", Error);
        return HRESULT_FROM_WIN32(Error);
    }

    if (State->RdpWindow.Dialog != NULL)
    {
        ChildSession_RdpWindowDestroy(&State->RdpWindow);
    }
    Resolution = ChildSession_GetSelectedResolution(State);
    Result = ChildSession_RdpWindowCreate(Window,
                                          Resolution,
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
    if (CreateChildSession)
    {
        State->OwnsChildSession = TRUE;
    }
    IO_ConPrintF("Remote Desktop resolution: %lu x %lu\n",
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
    if (State->RdpWindow.Dialog == NULL)
    {
        return S_FALSE;
    }
    ChildSession_RdpWindowDestroy(&State->RdpWindow);
    ChildSession_SetOperation(State, L"Remote Desktop window closed and disconnected");
    IO_ConPrintF("Remote Desktop window closed and disconnected\n");
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

    if (State->RdpWindow.Dialog != NULL)
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
    ChildSession_ReportOperation(State, L"Child session logged off");
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
    W32ERROR Error = ERROR_SUCCESS;
    BOOLEAN Enabled;

    if (Index >= ARRAYSIZE(State->ConfigurationButtons) || State->RdpWindow.ConnectPending)
    {
        return;
    }
    if (MessageBoxW(Window,
                    L"Apply this machine-wide configuration change?\n\n"
                    L"It will remain in effect after this demo closes. Credential delegation permits "
                    L"default credentials for TERMSRV/localhost; disabling Hello-only allows password sign-in.\n\n"
                    L"Removing delegation removes only this helper's entries, not other policy entries. "
                    L"An existing connection may need to be reconnected.",
                    L"Change child-session configuration",
                    MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES)
    {
        return;
    }

    Enabled = g_ChildSessionConfigurationButtons[Index].Enabled;
    switch (g_ChildSessionConfigurationButtons[Index].Setting)
    {
        case ChildSessionSettingEnabled:
            Error = (WinStationEnableChildSessions(Enabled) ? ERROR_SUCCESS : Err_GetLastError());
            if (Error == ERROR_SUCCESS)
            {
                // Explicit configuration supersedes the temporary connection-time change.
                State->Configuration->Original.Enabled = Enabled;
                State->Configuration->EnabledTouched = FALSE;
            }
            break;
        case ChildSessionSettingCredentialDelegation:
            Error = ChildSession_SetChildSessionCredentialDelegation(Enabled);
            break;
        case ChildSessionSettingHelloOnly:
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
    HFONT Font;

    if (UI_CreateDefaultFont(&Font, 0) != ERROR_SUCCESS)
    {
        return;
    }
    for (HWND Control = GetWindow(Window, GW_CHILD); Control != NULL; Control = GetWindow(Control, GW_HWNDNEXT))
    {
        UI_SetWindowFont(Control, Font, TRUE);
    }
    if (State->Font != NULL)
    {
        DeleteObject(State->Font);
    }
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
    const auto Scale = [Dpi](LONG Value)
    {
        return MulDiv(Value, Dpi, 96);
    };
    const LONG Margin = Scale(12), TextHeight = Scale(24), ButtonTop = Scale(240);
    const LONG ButtonWidth = Scale(132), ButtonHeight = Scale(32), ButtonGap = Scale(8);

    if (!GetClientRect(Window, &ClientRect))
    {
        return;
    }
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

                Str_PrintfW(Text,
                            L"Remote Desktop size: %lu x %lu",
                            (ULONG)WParam,
                            (ULONG)LParam);
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
                if (WParam == MSTSCAXEVENT_DISPID_CONNECTING || WParam == MSTSCAXEVENT_DISPID_WARNING ||
                    WParam == MSTSCAXEVENT_DISPID_INTERNALDIALOGDISPLAYED ||
                    WParam == MSTSCAXEVENT_DISPID_INTERNALDIALOGDISMISSED ||
                    WParam == MSTSCAXEVENT_DISPID_LOGONERROR)
                {
                    WCHAR Text[256];
                    PCWSTR EventName = WParam == MSTSCAXEVENT_DISPID_CONNECTING ? L"Connecting" :
                                       WParam == MSTSCAXEVENT_DISPID_WARNING ? L"Warning" :
                                       WParam == MSTSCAXEVENT_DISPID_INTERNALDIALOGDISPLAYED ?
                                           L"Authentication dialog displayed" :
                                       WParam == MSTSCAXEVENT_DISPID_INTERNALDIALOGDISMISSED ?
                                           L"Authentication dialog dismissed" :
                                                       L"Logon event";

                    Str_PrintfW(Text,
                                L"Remote Desktop: %s (code %ld)",
                                EventName,
                                (LONG)LParam);
                    ChildSession_SetOperation(State, Text);
                    IO_ConPrintF("%ls\n", Text);
                    return 0;
                }
                if (WParam == MSTSCAXEVENT_DISPID_CONNECTED || WParam == MSTSCAXEVENT_DISPID_LOGINCOMPLETE)
                {
                    SetWindowTextW(Window,
                                   WParam == MSTSCAXEVENT_DISPID_CONNECTED ?
                                       CHILD_SESSION_WINDOW_TITLE L" - Connected" :
                                       CHILD_SESSION_WINDOW_TITLE L" - Active");
                    ChildSession_SetOperation(State,
                                              WParam == MSTSCAXEVENT_DISPID_CONNECTED ?
                                                  L"Remote Desktop connected; waiting for logon" :
                                                  L"Child session is active");
                    IO_ConPrintF(WParam == MSTSCAXEVENT_DISPID_CONNECTED ?
                                        "Remote Desktop connected\n" :
                                        "Child session is active\n");
                } else
                {
                    WCHAR Text[256];

                    State->DisconnectReason = (LONG)LParam;
                    SetWindowTextW(Window, CHILD_SESSION_WINDOW_TITLE);
                    Str_PrintfW(Text,
                                WParam == MSTSCAXEVENT_DISPID_FATALERROR ?
                                    L"Remote Desktop fatal error: %ld" :
                                    L"Remote Desktop disconnected: reason %ld",
                                State->DisconnectReason);
                    ChildSession_SetOperation(State, Text);
                    IO_ConPrintF(WParam == MSTSCAXEVENT_DISPID_FATALERROR ?
                                        "Remote Desktop fatal error %ld\n" :
                                        "Remote Desktop disconnected with reason %ld\n",
                                    State->DisconnectReason);
                    ChildSession_RdpWindowDestroy(&State->RdpWindow);
                }
                ChildSession_RefreshWindow(State);
                return 0;
            case CHILD_SESSION_MESSAGE_RDP_CLOSE:
                ChildSession_ReportOperation(State, L"Remote Desktop window closed and disconnected");
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
ChildSession_CreateControls(
    _In_ HWND Window,
    _Inout_ PCHILD_SESSION_WINDOW_STATE State)
{
    const struct
    {
        HWND* Window;
        PCWSTR Class;
        PCWSTR Text;
        ULONG Style;
        ULONG Id;
    } Controls[] = {
        { &State->ConfigurationText, L"STATIC", L"Configuration: loading...", SS_LEFT, 0 },
        { &State->SessionText, L"STATIC", L"Child session: loading...", SS_LEFT, 0 },
        { &State->OperationText, L"STATIC", L"Ready", SS_LEFT, 0 },
        { &State->ResolutionText, L"STATIC", L"Resolution:", SS_LEFT, 0 },
        { &State->ResolutionCombo, L"COMBOBOX", NULL, WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, 0 },
        { &State->RefreshButton, L"BUTTON", L"Refresh", WS_TABSTOP, CHILD_SESSION_BUTTON_REFRESH },
        { &State->ConnectButton, L"BUTTON", L"Connect", WS_TABSTOP | BS_DEFPUSHBUTTON, CHILD_SESSION_BUTTON_CONNECT },
        { &State->DisconnectButton, L"BUTTON", L"Disconnect", WS_TABSTOP, CHILD_SESSION_BUTTON_DISCONNECT },
        { &State->LogoffButton, L"BUTTON", L"Log off", WS_TABSTOP, CHILD_SESSION_BUTTON_LOGOFF },
        { &State->CloseButton, L"BUTTON", L"Close", WS_TABSTOP, CHILD_SESSION_BUTTON_CLOSE }
    };
    const auto CreateControl = [Window](HWND* Control, PCWSTR Class, PCWSTR Text, ULONG Style, ULONG Id)
    {
        *Control = CreateWindowExW(0,
                                   Class,
                                   Text,
                                   WS_CHILD | WS_VISIBLE | Style,
                                   0,
                                   0,
                                   0,
                                   0,
                                   Window,
                                   (HMENU)(ULONG_PTR)Id,
                                   GetModuleHandleW(NULL),
                                   NULL);
        return *Control != NULL ? S_OK : HRESULT_FROM_WIN32(Err_GetLastError());
    };
    HRESULT Result;

    for (const auto& Control : Controls)
    {
        Result = CreateControl(Control.Window, Control.Class, Control.Text, Control.Style, Control.Id);
        if (FAILED(Result))
        {
            return Result;
        }
    }
    for (ULONG Index = 0; Index < ARRAYSIZE(g_ChildSessionConfigurationButtons); Index++)
    {
        Result = CreateControl(&State->ConfigurationButtons[Index],
                               L"BUTTON",
                               g_ChildSessionConfigurationButtons[Index].Text,
                               WS_TABSTOP,
                               CHILD_SESSION_BUTTON_CONFIGURATION + Index);
        if (FAILED(Result))
        {
            return Result;
        }
    }
    return CreateControl(&State->ConfigurationNote,
                         L"STATIC",
                         L"Configuration buttons save machine-wide changes; closing the demo does not undo them.",
                         SS_LEFT,
                         0);
}

static
HRESULT
ChildSession_RunWindow(
    _In_ PCHILD_SESSION_CONFIGURATION_STATE Configuration)
{
    WNDCLASSEXW ConsoleWindowClass = { sizeof(ConsoleWindowClass) };
    CHILD_SESSION_WINDOW_STATE WindowState = { 0 };
    HMODULE Instance = GetModuleHandleW(NULL);
    HWND Window = NULL;
    DPI_AWARENESS_CONTEXT PreviousDpiContext;
    MSG Message;
    HRESULT Result;
    W32ERROR Error;
    BOOL MessageResult;
    BOOLEAN Initialized = FALSE;
    BOOLEAN ConsoleClassRegistered = FALSE;

    WindowState.Configuration = Configuration;
    IO_ConPrintF("Remote Desktop process PMv2 before initialization: %s\n",
                    AreDpiAwarenessContextsEqual(GetDpiAwarenessContextForProcess(GetCurrentProcess()),
                                                DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) ? "yes" : "no");
    // RDP-created threads must inherit PMv2 as well as the host UI thread.
    if (!AreDpiAwarenessContextsEqual(GetDpiAwarenessContextForProcess(GetCurrentProcess()),
                                     DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) &&
        !SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
    {
        Error = Err_GetLastError();
        IO_ConPrintF("Set process PMv2 failed with Win32 error %lu\n", Error);
        return HRESULT_FROM_WIN32(Error);
    }
    PreviousDpiContext = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    if (PreviousDpiContext == NULL)
    {
        return HRESULT_FROM_WIN32(Err_GetLastError());
    }
    IO_ConPrintF("Remote Desktop DPI context: process PMv2=%s, thread PMv2=%s\n",
                    AreDpiAwarenessContextsEqual(GetDpiAwarenessContextForProcess(GetCurrentProcess()),
                                                DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) ? "yes" : "no",
                    AreDpiAwarenessContextsEqual(GetThreadDpiAwarenessContext(),
                                                DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) ? "yes" : "no");
    Result = OleInitialize(NULL);
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Initialized = TRUE;
    ConsoleWindowClass.style = CS_HREDRAW | CS_VREDRAW;
    ConsoleWindowClass.lpfnWndProc = ChildSession_WindowProc;
    ConsoleWindowClass.hInstance = Instance;
    ConsoleWindowClass.hCursor = LoadCursorW(NULL, IDC_ARROW);
    ConsoleWindowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    ConsoleWindowClass.lpszClassName = CHILD_SESSION_CONSOLE_WINDOW_CLASS;
    if (RegisterClassExW(&ConsoleWindowClass) == 0)
    {
        Result = HRESULT_FROM_WIN32(Err_GetLastError());
        goto Cleanup;
    }
    ConsoleClassRegistered = TRUE;
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
    if (Window == NULL)
    {
        Result = HRESULT_FROM_WIN32(Err_GetLastError());
        goto Cleanup;
    }
    Result = ChildSession_CreateControls(Window, &WindowState);
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    ChildSession_UpdateConsoleFont(Window, &WindowState);
    SetWindowPos(Window,
                 NULL,
                 0,
                 0,
                 MulDiv(1120, GetDpiForWindow(Window), 96),
                 MulDiv(360, GetDpiForWindow(Window), 96),
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
    Error = ChildSession_RefreshWindow(&WindowState);
    if (Error != ERROR_SUCCESS)
    {
        ChildSession_SetWin32Error(&WindowState, L"Initial status refresh", Error);
    }
    SetTimer(Window, 1, 1000, NULL);
    ShowWindow(Window, SW_SHOW);
    UpdateWindow(Window);
    SetFocus(WindowState.ConnectButton);
    while ((MessageResult = GetMessageW(&Message, NULL, 0, 0)) > 0)
    {
        if (WindowState.RdpWindow.Dialog != NULL &&
            UI_RdpDialogTranslateMessage(WindowState.RdpWindow.Dialog, &Message))
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
    if (MessageResult == -1)
    {
        Result = HRESULT_FROM_WIN32(Err_GetLastError());
    }

Cleanup:
    ChildSession_RdpWindowDestroy(&WindowState.RdpWindow);
    if (WindowState.OwnsChildSession)
    {
        Error = ChildSession_LogoffChildSession(TRUE);
        if (Error != ERROR_SUCCESS)
        {
            IO_ConPrintF("Child session cleanup failed with Win32 error %lu\n", Error);
            if (SUCCEEDED(Result))
            {
                Result = HRESULT_FROM_WIN32(Error);
            }
        } else
        {
            IO_ConPrintF("Sample-created child session logged off during cleanup\n");
        }
    }
    if (Window != NULL && IsWindow(Window))
    {
        DestroyWindow(Window);
    }
    if (WindowState.Font != NULL)
    {
        DeleteObject(WindowState.Font);
    }
    if (ConsoleClassRegistered)
    {
        UnregisterClassW(CHILD_SESSION_CONSOLE_WINDOW_CLASS, Instance);
    }
    if (Initialized)
    {
        OleUninitialize();
    }
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
        IO_ConPrintF("usage: ChildSession.exe\n");
        return E_INVALIDARG;
    }
    Error = ChildSession_PrepareConfiguration(&Configuration);
    if (Error != ERROR_SUCCESS)
    {
        IO_ConPrintF("Child session preparation failed with Win32 error %lu\n", Error);
        Result = HRESULT_FROM_WIN32(Error);
        goto CleanupConfiguration;
    }
    Result = ChildSession_RunWindow(&Configuration);
    if (FAILED(Result))
    {
        IO_ConPrintF("Child session console failed with HRESULT 0x%08lX\n", (ULONG)Result);
    }

CleanupConfiguration:
    CleanupError = ChildSession_RestoreConfiguration(&Configuration);
    if (CleanupError != ERROR_SUCCESS)
    {
        IO_ConPrintF("Child session configuration restore failed with Win32 error %lu\n", CleanupError);
        if (SUCCEEDED(Result))
        {
            Result = HRESULT_FROM_WIN32(CleanupError);
        }
    }
    return Result;
}
