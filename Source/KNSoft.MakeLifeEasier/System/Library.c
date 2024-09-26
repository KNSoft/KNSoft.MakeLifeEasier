#include "../MakeLifeEasier.inl"

static PVOID g_apSysLib[SysLibMax] = { NULL };

static const UNICODE_STRING g_ausSysDllNames[] = {
    // RTL_CONSTANT_STRING(L"ntdll.dll"), // NtDll.dll always is the first module initialized
    RTL_CONSTANT_STRING(L"kernel32.dll"),
    RTL_CONSTANT_STRING(L"user32.dll"),
    RTL_CONSTANT_STRING(L"Comdlg32.dll"),
    RTL_CONSTANT_STRING(L"Ole32.dll"),
    RTL_CONSTANT_STRING(L"UxTheme.dll"),
    RTL_CONSTANT_STRING(L"Dwmapi.dll"),
    RTL_CONSTANT_STRING(L"Shcore.dll"),
    RTL_CONSTANT_STRING(L"Ws2_32.dll"),
    RTL_CONSTANT_STRING(L"Winmm.dll")
};

NTSTATUS
NTAPI
Sys_LoadDll(
    _In_ SYS_LIB_INDEX SysLib,
    _Out_ PVOID* DllBase)
{
    NTSTATUS Status;

    if (SysLib >= 0 && SysLib < SysLibMax)
    {
        if (g_apSysLib[SysLib] == NULL)
        {
            if (SysLib == SysLibNtDll)
            {
                g_apSysLib[SysLib] = NtGetNtdllBase();
            } else
            {
                Status = LdrLoadDll(NULL, NULL, (PUNICODE_STRING)&g_ausSysDllNames[SysLib - 1], &g_apSysLib[SysLib]);
                if (!NT_SUCCESS(Status))
                {
                    return Status;
                }
            }
        }
        *DllBase = g_apSysLib[SysLib];
        return STATUS_SUCCESS;
    } else
    {
        return STATUS_INVALID_PARAMETER_1;
    }
}
