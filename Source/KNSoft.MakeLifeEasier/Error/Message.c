#include "../MakeLifeEasier.inl"

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
    UI_MsgBox(Owner, Err_GetNtStatusInfo(Status), Title, Type | MB_OK);
}
