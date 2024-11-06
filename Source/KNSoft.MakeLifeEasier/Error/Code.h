#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

/* See also RtlNtStatusToDosErrorNoTeb */
FORCEINLINE
ULONG
Err_NtStatusToWin32Error(
    _In_ NTSTATUS Status)
{
    return NT_FACILITY(Status) == FACILITY_NTWIN32 ? NT_CODE(Status) : RtlNtStatusToDosErrorNoTeb(Status);
}

/* See also HRESULT_FROM_NT and wil::NtStatusToHr */
FORCEINLINE
HRESULT
Err_NtStatusToHr(
    _In_ NTSTATUS Status)
{
    ULONG Win32Error;

    if (NT_SUCCESS(Status))
    {
        return S_OK;
    } else if (Status == STATUS_NO_MEMORY)
    {
        return E_OUTOFMEMORY;
    }
    Win32Error = Err_NtStatusToWin32Error(Status);
    if (Win32Error != 0 && Win32Error != ERROR_MR_MID_NOT_FOUND)
    {
        return HRESULT_FROM_WIN32(Win32Error);
    }
    return HRESULT_FROM_NT(Status);
}

/* See also wil::HrToNtStatus */
MLE_API
NTSTATUS
NTAPI
Err_HrToNtStatus(
    _In_ HRESULT Hr);

EXTERN_C_END
