#pragma once

#include "../MakeLifeEasier.h"

#include "../PE/Resource.h"
#include "../System/APIThunks.h"
#include "../UI/DialogBox/DialogBox.h"

EXTERN_C_START

#pragma region Error Message

FORCEINLINE
_Success_(return != NULL)
PCWSTR
Err_GetWin32ErrorInfo(
    _In_ ULONG Win32Error)
{
    PVOID DllHandle = Sys_LoadDll(SysDll_kernel32);

    return DllHandle != NULL ? PE_FindMessage(DllHandle, 0, Win32Error) : NULL;
}

FORCEINLINE
_Success_(return != NULL)
PCWSTR
Err_GetNtStatusInfo(
    _In_ NTSTATUS Status)
{
    PVOID DllHandle = Sys_LoadDll(SysDll_ntdll);

    if (NT_FACILITY(Status) == FACILITY_NTWIN32)
    {
        return Err_GetWin32ErrorInfo(NT_CODE(Status));
    }

    return DllHandle != NULL ? PE_FindMessage(DllHandle, 0, Status) : NULL;
}

FORCEINLINE
_Success_(return != NULL)
PCWSTR
Err_GetHrInfo(
    _In_ HRESULT Hr)
{
    ULONG Facility, Severity, Code;

    Facility = HRESULT_FACILITY(Hr);
    Severity = HRESULT_SEVERITY(Hr);
    Code = HRESULT_CODE(Hr);

    if (Facility == FACILITY_WIN32 && Severity == SEVERITY_ERROR)
    {
        return Err_GetWin32ErrorInfo(Code);
    } else if ((ULONG)Hr & FACILITY_NT_BIT)
    {
        return Err_GetNtStatusInfo((NTSTATUS)((ULONG)Hr & ~FACILITY_NT_BIT));
    }

    return Err_GetWin32ErrorInfo((ULONG)Hr);
}

#pragma endregion

#pragma region Error Message Box

FORCEINLINE
VOID
Err_Win32ErrorMessageBox(
    _In_opt_ HWND Owner,
    _In_opt_ PCWSTR Title,
    _In_ ULONG Win32Error)
{
    UI_MsgBox(Owner,
              Err_GetWin32ErrorInfo(Win32Error),
              Title,
              (Win32Error == ERROR_SUCCESS ? MB_ICONINFORMATION : MB_ICONERROR) | MB_OK);
}

FORCEINLINE
VOID
Err_NtStatusMessageBox(
    _In_opt_ HWND Owner,
    _In_opt_ PCWSTR Title,
    _In_ NTSTATUS Status)
{
    UINT Type;

    if (NT_ERROR(Status))
    {
        Type = MB_ICONERROR;
    } else if (NT_WARNING(Status))
    {
        Type = MB_ICONWARNING;
    } else
    {
        Type = MB_ICONINFORMATION;
    }
    UI_MsgBox(Owner, Err_GetNtStatusInfo(Status), Title, Type | MB_OK);
}

FORCEINLINE
VOID
Err_HrMessageBox(
    _In_opt_ HWND Owner,
    _In_opt_ PCWSTR Title,
    _In_ HRESULT Hr)
{
    UI_MsgBox(Owner,
              Err_GetHrInfo(Hr),
              Title,
              (HRESULT_SEVERITY(Hr) == SEVERITY_ERROR ? MB_ICONERROR : MB_ICONINFORMATION) | MB_OK);
}

#pragma endregion

EXTERN_C_END
