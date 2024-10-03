#include "../MakeLifeEasier.inl"

ULONG
NTAPI
Err_NtStatusToWin32Error(
    _In_ NTSTATUS Status)
{
    return NT_FACILITY(Status) == FACILITY_NTWIN32 ? NT_CODE(Status) : RtlNtStatusToDosErrorNoTeb(Status);
}

_Success_(return != NULL)
PCWSTR
NTAPI
Err_GetWin32ErrorInfo(
    _In_ ULONG Win32Error)
{
    PVOID DllHandle;

    return NT_SUCCESS(Sys_LoadDll(SysLibKernel32, &DllHandle)) ? PE_FindMessage(DllHandle, 0, Win32Error) : NULL;
}

_Success_(return != NULL)
PCWSTR
NTAPI
Err_GetNtStatusInfo(
    _In_ NTSTATUS Status)
{
    PVOID DllHandle;

    if (NT_FACILITY(Status) == FACILITY_NTWIN32)
    {
        return Err_GetWin32ErrorInfo(NT_CODE(Status));
    }

    return NT_SUCCESS(Sys_LoadDll(SysLibNtDll, &DllHandle)) ? PE_FindMessage(DllHandle, 0, Status) : NULL;
}

_Success_(return != NULL)
PCWSTR
NTAPI
Err_GetHResultInfo(
    _In_ HRESULT HResult)
{
    ULONG Facility, Code;

    Facility = HRESULT_FACILITY(HResult);
    Code = HRESULT_CODE(HResult);

    if (Facility == FACILITY_WIN32)
    {
        return Err_GetWin32ErrorInfo(Code);
    } else if ((ULONG)HResult & FACILITY_NT_BIT)
    {
        return Err_GetNtStatusInfo(HResult & ~FACILITY_NT_BIT);
    }

    return NULL;
}

VOID
NTAPI
Error_Win32ErrorMessageBox(
    _In_opt_ HWND Owner,
    _In_opt_ PCWSTR Title,
    _In_ ULONG Win32Error)
{
    Dlg_MsgBox(Owner,
               Err_GetWin32ErrorInfo(Win32Error),
               Title,
               (Win32Error == ERROR_SUCCESS ? MB_ICONINFORMATION : MB_ICONERROR) | MB_OK);
}

VOID
NTAPI
Error_NtStatusMessageBox(
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
    Dlg_MsgBox(Owner, Err_GetNtStatusInfo(Status), Title, Type | MB_OK);
}

VOID
NTAPI
Error_HResultMessageBox(
    _In_opt_ HWND Owner,
    _In_opt_ PCWSTR Title,
    _In_ HRESULT HResult)
{
    Dlg_MsgBox(Owner,
               Err_GetHResultInfo(HResult),
               Title,
               (HRESULT_SEVERITY(HResult) == SEVERITY_ERROR ? MB_ICONERROR : MB_ICONINFORMATION) | MB_OK);
}
