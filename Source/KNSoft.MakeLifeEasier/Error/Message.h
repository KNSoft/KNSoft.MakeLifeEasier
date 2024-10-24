#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

FORCEINLINE
ULONG
Err_NtStatusToWin32Error(
    _In_ NTSTATUS Status)
{
    return NT_FACILITY(Status) == FACILITY_NTWIN32 ? NT_CODE(Status) : RtlNtStatusToDosErrorNoTeb(Status);
}

#pragma region Error Message

FORCEINLINE
_Success_(return != NULL)
PCWSTR
Err_GetWin32ErrorInfo(
    _In_ ULONG Win32Error)
{
    PVOID DllHandle;

    return NT_SUCCESS(Sys_LoadDll(SysLibKernel32, &DllHandle)) ? PE_FindMessage(DllHandle, 0, Win32Error) : NULL;
}

MLE_API
_Success_(return != NULL)
PCWSTR
NTAPI
Err_GetNtStatusInfo(
    _In_ NTSTATUS Status);

MLE_API
_Success_(return != NULL)
PCWSTR
NTAPI
Err_GetHResultInfo(
    _In_ HRESULT HResult);

#pragma endregion

#pragma region Error Message Box

FORCEINLINE
VOID
Error_Win32ErrorMessageBox(
    _In_opt_ HWND Owner,
    _In_opt_ PCWSTR Title,
    _In_ ULONG Win32Error)
{
    UI_MsgBox(Owner,
              Err_GetWin32ErrorInfo(Win32Error),
              Title,
              (Win32Error == ERROR_SUCCESS ? MB_ICONINFORMATION : MB_ICONERROR) | MB_OK);
}

MLE_API
VOID
NTAPI
Error_NtStatusMessageBox(
    _In_opt_ HWND Owner,
    _In_opt_ PCWSTR Title,
    _In_ NTSTATUS Status);

FORCEINLINE
VOID
Error_HResultMessageBox(
    _In_opt_ HWND Owner,
    _In_opt_ PCWSTR Title,
    _In_ HRESULT HResult)
{
    UI_MsgBox(Owner,
              Err_GetHResultInfo(HResult),
              Title,
              (HRESULT_SEVERITY(HResult) == SEVERITY_ERROR ? MB_ICONERROR : MB_ICONINFORMATION) | MB_OK);
}

#pragma endregion

EXTERN_C_END
