/* Load system APIs with cache, like what MSVCRT did */

#include "../MakeLifeEasier.inl"

CONST UNICODE_STRING Sys_DLLNames[SysDll_Max] = {
#define _APPLY(DllName) RTL_CONSTANT_STRING(_A2W(_CRT_STRINGIZE(DllName)".dll")),
    _SYS_APITHUNKS_APPLY_DLLS
#undef _APPLY
};

static DECLSPEC_POINTERALIGN _Interlocked_operand_ PVOID volatile g_aSysDllBases[SysDll_Max];

_Success_(return != NULL)
PVOID
NTAPI
Sys_LoadDll(
    _In_ SYS_DLL_INDEX SysDll)
{
    NTSTATUS Status;
    PVOID Base, NewBase;

    /* Get cached handle */
    Base = _InterlockedReadPointer((const volatile void**)&g_aSysDllBases[SysDll]);
    if (Base != NULL)
    {
        return Base != INVALID_HANDLE_VALUE ? Base : NULL;
    }

    /* Load system DLL */
    if (SysDll == SysDll_ntdll)
    {
        Status = PS_GetNtdllBase(&NewBase);
    } else
    {
        Status = PS_LoadDllFromSystemDir(NULL,
                                        (PUNICODE_STRING)&Sys_DLLNames[SysDll],
                                        &NewBase);
    }
    if (!NT_SUCCESS(Status))
    {
        _InterlockedExchangePointer(&g_aSysDllBases[SysDll], INVALID_HANDLE_VALUE);
        return NULL;
    }

    /* Dereference if it is loaded twice */
    if (_InterlockedExchangePointer(&g_aSysDllBases[SysDll], NewBase) == NewBase)
    {
        LdrUnloadDll(NewBase);
    }
    return NewBase;
}

static CONST UCHAR g_aSysFuncDlls[SysFunc_Max] = {
#define _APPLY(DLLName, FuncName, MinNtVer) _SYS_APITHUNKS_PREFIX_DLL(DLLName),
    _SYS_APITHUNKS_APPLY_FUNCTIONS
#undef _APPLY
};

static CONST ANSI_STRING g_aSysFuncNames[SysFunc_Max] = {
#define _APPLY(DLLName, FuncName, MinNtVer) RTL_CONSTANT_STRING(_CRT_STRINGIZE(FuncName)),
    _SYS_APITHUNKS_APPLY_FUNCTIONS
#undef _APPLY
};

static CONST ULONG g_aSysFuncMinNTVers[SysFunc_Max] = {
#define _APPLY(DLLName, FuncName, MinNtVer) MinNtVer,
    _SYS_APITHUNKS_APPLY_FUNCTIONS
#undef _APPLY
};

static DECLSPEC_POINTERALIGN _Interlocked_operand_ PVOID volatile g_aSysFuncAddrs[SysFunc_Max];

_Success_(return != NULL)
PVOID
NTAPI
Sys_LoadFunction(
    _In_ SYS_FUNCTION_INDEX SysFunction)
{
    PVOID Ptr;

    /* Get cached handle */
    Ptr = ReadPointerNoFence((_Interlocked_operand_ PVOID const volatile *)&g_aSysFuncAddrs[SysFunction]);
    if (Ptr != NULL)
    {
        return Ptr != INVALID_HANDLE_VALUE ? Ptr : NULL;
    }

    /* Get function address */
    if (!IS_NT_VERSION_GE(g_aSysFuncMinNTVers[SysFunction]))
    {
        goto _Fail;
    }
    Ptr = Sys_LoadDll(g_aSysFuncDlls[SysFunction]);
    if (Ptr == NULL)
    {
        goto _Fail;
    }
    if (!NT_SUCCESS(PS_GetProcAddress(Ptr, (PCANSI_STRING)&g_aSysFuncNames[SysFunction], &Ptr)))
    {
        goto _Fail;
    }
    _InterlockedExchangePointer(&g_aSysFuncAddrs[SysFunction], Ptr);
    return Ptr;

_Fail:
    _InterlockedExchangePointer(&g_aSysFuncAddrs[SysFunction], INVALID_HANDLE_VALUE);
    return NULL;
}
