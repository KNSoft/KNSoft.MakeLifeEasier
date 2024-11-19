#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

typedef struct _KNS_APPINFO
{
    PCWSTR AppName;
    PCWSTR Homepage;
} KNS_APPINFO, *PKNS_APPINFO;

extern KNS_APPINFO g_KNSAppInfo;

FORCEINLINE
VOID
KNS_Win32ErrorMessageBox(
    _In_opt_ HWND Owner,
    _In_ ULONG Win32Error)
{
    Err_Win32ErrorMessageBox(Owner, g_KNSAppInfo.AppName, Win32Error);
}

FORCEINLINE
VOID
KNS_NtStatusMessageBox(
    _In_opt_ HWND Owner,
    _In_ NTSTATUS Status)
{
    Err_NtStatusMessageBox(Owner, g_KNSAppInfo.AppName, Status);
}

FORCEINLINE
VOID
KNS_HrMessageBox(
    _In_opt_ HWND Owner,
    _In_ HRESULT Hr)
{
    Err_HrMessageBox(Owner, g_KNSAppInfo.AppName, Hr);
}

FORCEINLINE
W32ERROR
KNS_OpenHomepage(VOID)
{
    return Shell_Exec(g_KNSAppInfo.Homepage, NULL, L"open", SW_SHOWNORMAL, NULL);
}

EXTERN_C_END
