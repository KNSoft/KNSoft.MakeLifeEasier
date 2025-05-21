#include "../MakeLifeEasier.inl"

static RTL_SRWLOCK g_Lock = RTL_SRWLOCK_INIT;

W32ERROR
NTAPI
PE_SymInitialize(
    _In_opt_ PCWSTR UserSearchPath,
    _In_ BOOL fInvadeProcess)
{
    W32ERROR Ret;

    RtlAcquireSRWLockExclusive(&g_Lock);
    Ret = SymInitializeW(NtReadTeb(ClientId.UniqueProcess),
                         UserSearchPath,
                         fInvadeProcess) ? ERROR_SUCCESS : Err_GetLastError();
    RtlReleaseSRWLockExclusive(&g_Lock);

    return Ret;
}

W32ERROR
NTAPI
PE_SymCleanup(VOID)
{
    W32ERROR Ret;

    RtlAcquireSRWLockExclusive(&g_Lock);
    Ret = SymCleanup(NtReadTeb(ClientId.UniqueProcess)) ? ERROR_SUCCESS : Err_GetLastError();
    RtlReleaseSRWLockExclusive(&g_Lock);

    return Ret;
}

W32ERROR
NTAPI
PE_SymSetOptions(
    _In_ DWORD SymOptions,
    _Out_opt_ PDWORD OldSymOptions)
{
    W32ERROR Ret;
    DWORD OldOptions;

    RtlAcquireSRWLockExclusive(&g_Lock);

    OldOptions = SymGetOptions();
    if (SymSetOptions(SymOptions) != SymOptions)
    {
        SymSetOptions(OldOptions);
        Ret = ERROR_INVALID_PARAMETER;
        goto _Exit;
    }
    if (OldSymOptions != NULL)
    {
        *OldSymOptions = OldOptions;
    }
    Ret = ERROR_SUCCESS;

_Exit:
    RtlReleaseSRWLockExclusive(&g_Lock);
    return Ret;
}

W32ERROR
NTAPI
PE_SymLoadModule(
    _In_opt_ HANDLE hFile,
    _In_opt_ PCWSTR ImageName,
    _In_opt_ PCWSTR ModuleName,
    _In_ DWORD64 BaseOfDll,
    _In_ DWORD DllSize,
    _In_opt_ PMODLOAD_DATA Data,
    _In_opt_ DWORD Flags,
    _Out_opt_ PDWORD64 BaseAddress)
{
    DWORD64 Base;

    RtlAcquireSRWLockExclusive(&g_Lock);
    Base = SymLoadModuleExW(NtReadTeb(ClientId.UniqueProcess),
                            hFile,
                            ImageName,
                            ModuleName,
                            BaseOfDll,
                            DllSize,
                            Data,
                            Flags);
    RtlReleaseSRWLockExclusive(&g_Lock);
    if (BaseAddress != NULL)
    {
        *BaseAddress = Base;
    }

    return Base != 0 ? ERROR_SUCCESS : Err_GetLastError();
}

W32ERROR
NTAPI
PE_SymUnloadModule(
    _In_ DWORD64 BaseOfDll)
{
    W32ERROR Ret;

    RtlAcquireSRWLockExclusive(&g_Lock);
    Ret = SymUnloadModule64(NtReadTeb(ClientId.UniqueProcess), BaseOfDll) ? ERROR_SUCCESS : Err_GetLastError();
    RtlReleaseSRWLockExclusive(&g_Lock);

    return Ret;
}

W32ERROR
NTAPI
PE_SymFromAddr(
    _In_ DWORD64 Address,
    _Out_opt_ PDWORD64 Displacement,
    _Inout_ PSYMBOL_INFOW Symbol)
{
    W32ERROR Ret;

    RtlAcquireSRWLockExclusive(&g_Lock);
    Ret = SymFromAddrW(NtReadTeb(ClientId.UniqueProcess),
                       Address,
                       Displacement,
                       Symbol) ? ERROR_SUCCESS : Err_GetLastError();
    RtlReleaseSRWLockExclusive(&g_Lock);

    return Ret;
}

