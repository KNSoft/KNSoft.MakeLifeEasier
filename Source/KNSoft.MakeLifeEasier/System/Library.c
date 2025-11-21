#include "../MakeLifeEasier.inl"

typedef struct _SYS_LIB
{
    CONST UNICODE_STRING Name;
    _Interlocked_operand_ PVOID volatile Base;
    NTSTATUS Status;
} SYS_LIB, *PSYS_LIB;

static SYS_LIB g_aSysDlls[SysLibMax] = {
    { RTL_CONSTANT_STRING(L"ntdll.dll"), NULL, STATUS_SUCCESS },
    { RTL_CONSTANT_STRING(L"kernel32.dll"), NULL, STATUS_SUCCESS },
    { RTL_CONSTANT_STRING(L"user32.dll"), NULL, STATUS_SUCCESS },
    { RTL_CONSTANT_STRING(L"Comdlg32.dll"), NULL, STATUS_SUCCESS },
    { RTL_CONSTANT_STRING(L"Ole32.dll"), NULL, STATUS_SUCCESS },
    { RTL_CONSTANT_STRING(L"UxTheme.dll"), NULL, STATUS_SUCCESS },
    { RTL_CONSTANT_STRING(L"Dwmapi.dll"), NULL, STATUS_SUCCESS },
    { RTL_CONSTANT_STRING(L"Shcore.dll"), NULL, STATUS_SUCCESS },
    { RTL_CONSTANT_STRING(L"Ws2_32.dll"), NULL, STATUS_SUCCESS },
    { RTL_CONSTANT_STRING(L"Winmm.dll"), NULL, STATUS_SUCCESS },
};

_STATIC_ASSERT(SysLibNtDll == 0);
_STATIC_ASSERT(SysLibMax == ARRAYSIZE(g_aSysDlls));

static RTL_SRWLOCK g_SysDllsLock = RTL_SRWLOCK_INIT;

_Success_(return != NULL)
NTSTATUS
NTAPI
Sys_GetNtdllBase(
    _Out_ PVOID* NtdllBase)
{
    /* Get the first loaded entry */
    PLDR_DATA_TABLE_ENTRY Entry = CONTAINING_RECORD(NtCurrentPeb()->Ldr->InInitializationOrderModuleList.Flink,
                                                    LDR_DATA_TABLE_ENTRY,
                                                    InInitializationOrderLinks);

    /* May be replaced by honey pot by very few tamper security softwares */
    if (RtlEqualUnicodeString(&Entry->BaseDllName, (PUNICODE_STRING)&g_aSysDlls[SysLibNtDll].Name, TRUE))
    {
        *NtdllBase = Entry->DllBase;
        return STATUS_SUCCESS;
    }

    /* Fallback to LdrGetDllHandleEx */
    return LdrGetDllHandleEx(LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT,
                             NULL,
                             NULL,
                             (PUNICODE_STRING)&g_aSysDlls[SysLibNtDll].Name,
                             NtdllBase);
}

NTSTATUS
NTAPI
Sys_LoadDll(
    _In_ SYS_LIB_INDEX SysLib,
    _Out_ PVOID* DllBase)
{
    NTSTATUS Status;
    PVOID Base;

    /* Check input parameter */
    if ((ULONG)SysLib >= SysLibMax)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    /* Get cached handle */
    RtlAcquireSRWLockShared(&g_SysDllsLock);
    Base = g_aSysDlls[SysLib].Base;
    if (Base != NULL)
    {
        *DllBase = Base;
        RtlReleaseSRWLockShared(&g_SysDllsLock);
        return STATUS_SUCCESS;
    }

    /* Return error code if initialization failed */
    Status = g_aSysDlls[SysLib].Status;
    RtlReleaseSRWLockShared(&g_SysDllsLock);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Load system DLL */
    RtlAcquireSRWLockExclusive(&g_SysDllsLock);
    if (SysLib == SysLibNtDll)
    {
        Status = Sys_GetNtdllBase(&Base);
    } else
    {
        Status = LdrLoadDll((PCWSTR)(LDR_PATH_IS_FLAGS | LDR_PATH_SEARCH_SYSTEM32),
                            NULL,
                            (PUNICODE_STRING)&g_aSysDlls[SysLib].Name,
                            &Base);
    }
    g_aSysDlls[SysLib].Base = Base;
    g_aSysDlls[SysLib].Status = Status;
    RtlReleaseSRWLockExclusive(&g_SysDllsLock);

    *DllBase = Base;
    return Status;
}
