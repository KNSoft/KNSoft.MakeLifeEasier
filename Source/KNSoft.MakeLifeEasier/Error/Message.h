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

MLE_API
_Success_(return != NULL)
PCWSTR
NTAPI
Err_GetWin32ErrorInfo(
    _In_ ULONG Win32Error);

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

MLE_API
VOID
NTAPI
Error_Win32ErrorMessageBox(
    _In_opt_ HWND Owner,
    _In_opt_ PCWSTR Title,
    _In_ ULONG Win32Error);

MLE_API
VOID
NTAPI
Error_NtStatusMessageBox(
    _In_opt_ HWND Owner,
    _In_opt_ PCWSTR Title,
    _In_ NTSTATUS Status);

MLE_API
VOID
NTAPI
Error_HResultMessageBox(
    _In_opt_ HWND Owner,
    _In_opt_ PCWSTR Title,
    _In_ HRESULT HResult);

#pragma endregion

EXTERN_C_END
