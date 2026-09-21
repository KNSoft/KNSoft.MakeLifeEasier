#include "AbeDecrypt.h"

/*** UI helpers ***/

HWND g_MainWindow;
PNET_BROWSER_INFO g_Browsers;
ULONG g_BrowserCount;
static PNET_BROWSER_PROFILE g_Profiles;
static ULONG g_ProfileCount;

typedef struct _ABE_LAYOUT
{
    BOOL Ready;
    INT ClientWidth;
    INT ClientHeight;
    INT MinTrackWidth;
    INT MinTrackHeight;
} ABE_LAYOUT;

static ABE_LAYOUT g_Layout;

static INT
AbeRectWidth(
    _In_ const RECT* Rect)
{
    return Rect->right - Rect->left;
}

static INT
AbeRectHeight(
    _In_ const RECT* Rect)
{
    return Rect->bottom - Rect->top;
}

static BOOL
AbeGetChildRect(
    _In_ HWND Window,
    _In_ INT Id,
    _Out_ RECT* Rect)
{
    HWND Child = GetDlgItem(Window, Id);

    if (Child == NULL || !GetWindowRect(Child, Rect))
    {
        return FALSE;
    }
    MapWindowPoints(NULL, Window, (POINT*)Rect, 2);
    return TRUE;
}

static VOID
AbeMoveChild(
    _In_ INT Id,
    _In_ INT Left,
    _In_ INT Top,
    _In_ INT Width,
    _In_ INT Height)
{
    HWND Child = GetDlgItem(g_MainWindow, Id);

    if (Child != NULL)
    {
        SetWindowPos(Child,
                     NULL,
                     Left,
                     Top,
                     Width,
                     Height,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

static VOID
AbeCaptureLayout(
    _In_ HWND Window)
{
    RECT ClientRect, WindowRect;

    RtlZeroMemory(&g_Layout, sizeof(g_Layout));
    if (!GetClientRect(Window, &ClientRect) || !GetWindowRect(Window, &WindowRect))
    {
        return;
    }

    g_Layout.ClientWidth = AbeRectWidth(&ClientRect);
    g_Layout.ClientHeight = AbeRectHeight(&ClientRect);
    g_Layout.MinTrackWidth = AbeRectWidth(&WindowRect);
    g_Layout.MinTrackHeight = AbeRectHeight(&WindowRect);
    g_Layout.Ready = TRUE;
}

static VOID
AbeInitList(
    _In_ HWND List,
    _In_z_ const PCWSTR* Columns,
    _In_reads_z_(16) const INT* Widths)
{
    INT i;

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
        Column.cx = Widths[i];
        SendMessageW(List, LVM_INSERTCOLUMNW, i, (LPARAM)&Column);
    }
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
        SendMessageW(Combo, CB_ADDSTRING, 0, (LPARAM)g_Profiles[i].Name);
    }
    SendMessageW(Combo, CB_SETCURSEL, 0, 0);
}

/* keep the RC-designed layout and distribute resize deltas over the data panes */
static VOID
AbeLayout(
    _In_ INT ClientWidth,
    _In_ INT ClientHeight)
{
    RECT Button, CookieList, PasswordList, Status;
    INT DeltaWidth, DeltaHeight, Gap1, Gap2;
    INT Width;
    INT CookieHeight, PasswordHeight, StatusHeight, Remainder;
    INT PasswordTop, StatusTop;

    if (!g_Layout.Ready ||
        !AbeGetChildRect(g_MainWindow, IDC_GO_BUTTON, &Button) ||
        !AbeGetChildRect(g_MainWindow, IDC_COOKIE_LIST, &CookieList) ||
        !AbeGetChildRect(g_MainWindow, IDC_PASSWORD_LIST, &PasswordList) ||
        !AbeGetChildRect(g_MainWindow, IDC_STATUS_EDIT, &Status))
    {
        return;
    }

    DeltaWidth = ClientWidth - g_Layout.ClientWidth;
    DeltaHeight = ClientHeight - g_Layout.ClientHeight;
    if (DeltaWidth == 0 && DeltaHeight == 0)
    {
        return;
    }

    AbeMoveChild(IDC_GO_BUTTON,
                 Button.left + DeltaWidth,
                 Button.top,
                 AbeRectWidth(&Button),
                 AbeRectHeight(&Button));

    Width = AbeRectWidth(&CookieList) + DeltaWidth;
    if (Width < 1)
    {
        Width = 1;
    }

    Gap1 = PasswordList.top - CookieList.bottom;
    Gap2 = Status.top - PasswordList.bottom;
    CookieHeight = AbeRectHeight(&CookieList) + DeltaHeight / 3;
    PasswordHeight = AbeRectHeight(&PasswordList) + DeltaHeight / 3;
    StatusHeight = AbeRectHeight(&Status) + DeltaHeight / 3;
    Remainder = DeltaHeight % 3;
    if (Remainder > 0)
    {
        CookieHeight++;
        if (Remainder > 1)
        {
            PasswordHeight++;
        }
    } else if (Remainder < 0)
    {
        StatusHeight--;
        if (Remainder < -1)
        {
            PasswordHeight--;
        }
    }

    PasswordTop = CookieList.top + CookieHeight + Gap1;
    StatusTop = PasswordTop + PasswordHeight + Gap2;

    AbeMoveChild(IDC_COOKIE_LIST, CookieList.left, CookieList.top, Width, CookieHeight);
    AbeMoveChild(IDC_PASSWORD_LIST, PasswordList.left, PasswordTop, Width, PasswordHeight);
    AbeMoveChild(IDC_STATUS_EDIT, Status.left, StatusTop, Width, StatusHeight);
    g_Layout.ClientWidth = ClientWidth;
    g_Layout.ClientHeight = ClientHeight;
}

INT_PTR CALLBACK
AbeDialogProc(
    _In_ HWND Window,
    _In_ UINT Message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    switch (Message)
    {
        case WM_INITDIALOG:
        {
            static const PCWSTR CookieColumns[] = { L"Version", L"Domain", L"Name", L"Value", NULL };
            static const INT CookieWidths[] = { 60, 180, 140, 400 };
            static const PCWSTR PasswordColumns[] = { L"Version", L"Site", L"Username", L"Password", NULL };
            static const INT PasswordWidths[] = { 60, 220, 140, 300 };
            ULONG i;

            g_MainWindow = Window;

            AbeInitList(GetDlgItem(Window, IDC_COOKIE_LIST), CookieColumns, CookieWidths);
            AbeInitList(GetDlgItem(Window, IDC_PASSWORD_LIST), PasswordColumns, PasswordWidths);
            AbeCaptureLayout(Window);

            {
                HWND Combo = GetDlgItem(Window, IDC_METHOD_COMBO);

                for (i = 0; i < MethodMax; i++)
                {
                    SendMessageW(Combo, CB_ADDSTRING, 0, (LPARAM)AbeMethodNames[i]);
                }
                SendMessageW(Combo, CB_SETCURSEL, (WPARAM)MethodHijack, 0);
            }

            /* browsers; no default selection - profiles load on selection only */
            {
                NTSTATUS Status = Net_BrowserEnumerate(&g_Browsers, &g_BrowserCount);
                HWND Combo = GetDlgItem(Window, IDC_BROWSER_COMBO);

                if (NT_SUCCESS(Status))
                {
                    for (i = 0; i < g_BrowserCount; i++)
                    {
                        SendMessageW(Combo, CB_ADDSTRING, 0, (LPARAM)g_Browsers[i].Name);
                    }
                    if (g_BrowserCount == 0)
                    {
                        UI_SetDlgItemTextW(Window, IDC_STATUS_EDIT, L"No supported browser installations found");
                    } else
                    {
                        WCHAR Text[64];

                        Str_PrintfExW(Text, ARRAYSIZE(Text), L"Browsers found: %lu", g_BrowserCount);
                        UI_SetDlgItemTextW(Window, IDC_STATUS_EDIT, Text);
                    }
                } else
                {
                    WCHAR Text[96];

                    Str_PrintfExW(Text, ARRAYSIZE(Text), L"Browser enumeration failed: 0x%08lX", Status);
                    UI_SetDlgItemTextW(Window, IDC_STATUS_EDIT, Text);
                }
            }
            return TRUE;
        }
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
                        MessageBoxW(Window,
                                    L"Select a browser, a profile and a method first",
                                    L"AbeDecrypt",
                                    MB_ICONWARNING);
                        break;
                    }
                    /* allocate everything the worker needs up front, so it can
                       always report back and re-enable the UI */
                    Job = Mem_Alloc(sizeof(*Job));
                    if (Job == NULL)
                    {
                        break;
                    }
                    Job->Result = Mem_Alloc(sizeof(*Job->Result));
                    if (Job->Result == NULL)
                    {
                        Mem_Free(Job);
                        break;
                    }
                    Job->Browser = g_Browsers[BrowserIndex];
                    Job->Method = (ABE_METHOD)MethodIndex;
                    Str_CopyExW(Job->Profile, MAX_PATH, g_Profiles[ProfileIndex].Directory);

                    UI_SetDlgItemTextW(Window, IDC_STATUS_EDIT, L"Working...");
                    EnableWindow(GetDlgItem(Window, IDC_GO_BUTTON), FALSE);
                    if (!QueueUserWorkItem(AbeWorker, Job, WT_EXECUTELONGFUNCTION))
                    {
                        EnableWindow(GetDlgItem(Window, IDC_GO_BUTTON), TRUE);
                        Mem_Free(Job->Result);
                        Mem_Free(Job);
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        case ABE_WM_RESULT:
        {
            PABE_RESULT Result = (PABE_RESULT)lParam;

            AbeFillList(GetDlgItem(Window, IDC_COOKIE_LIST), Result->Cookies, Result->CookieCount);
            AbeFillList(GetDlgItem(Window, IDC_PASSWORD_LIST), Result->Passwords, Result->PasswordCount);
            UI_SetDlgItemTextW(Window, IDC_STATUS_EDIT, Result->Status);
            if (!Result->Ok)
            {
                MessageBeep(MB_ICONERROR);
            }
            EnableWindow(GetDlgItem(Window, IDC_GO_BUTTON), TRUE);
            Mem_Free(Result->Cookies);
            Mem_Free(Result->Passwords);
            Mem_Free(Result);
            return TRUE;
        }
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED)
            {
                AbeLayout(LOWORD(lParam), HIWORD(lParam));
            }
            return TRUE;
        case WM_GETMINMAXINFO:
        {
            MINMAXINFO* Info = (MINMAXINFO*)lParam;

            if (g_Layout.Ready)
            {
                Info->ptMinTrackSize.x = g_Layout.MinTrackWidth;
                Info->ptMinTrackSize.y = g_Layout.MinTrackHeight;
            }
            return TRUE;
        }
        case WM_CLOSE:
            DestroyWindow(Window);
            return TRUE;
        case WM_DESTROY:
            Mem_Free(g_Profiles);
            PostQuitMessage(0);
            return TRUE;
        default:
            break;
    }
    return FALSE;
}
