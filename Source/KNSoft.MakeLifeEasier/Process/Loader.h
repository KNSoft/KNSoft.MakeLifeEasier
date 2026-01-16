#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

FORCEINLINE
_Success_(return != NULL)
NTSTATUS
NTAPI
PS_GetNtdllBase(
    _Out_ PVOID* NtdllBase)
{
    /* Get the first initialized entry */
    PLDR_DATA_TABLE_ENTRY Entry = CONTAINING_RECORD(NtCurrentPeb()->Ldr->InInitializationOrderModuleList.Flink,
                                                    LDR_DATA_TABLE_ENTRY,
                                                    InInitializationOrderLinks);

    /* May be replaced by honey pot by very few tamper security softwares */
    if (RtlEqualUnicodeString(&Entry->BaseDllName, (PUNICODE_STRING)&Sys_DLLNames[SysDll_ntdll], TRUE))
    {
        *NtdllBase = Entry->DllBase;
        return STATUS_SUCCESS;
    }

    /* Fallback to LdrGetDllHandleEx */
    return LdrGetDllHandleEx(LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT,
                             NULL,
                             NULL,
                             (PUNICODE_STRING)&Sys_DLLNames[SysDll_ntdll],
                             NtdllBase);
}

/* LDR_PATH_SEARCH_SYSTEM32 since NT6.0/6.1 with KB2533623 */
FORCEINLINE
NTSTATUS
NTAPI
PS_LoadDllFromSystemDir(
    _In_opt_ PULONG DllCharacteristics,
    _In_ PUNICODE_STRING DllName,
    _Out_ PVOID* DllHandle)
{
    NTSTATUS Status;
    PCWSTR DllPath = IS_NT_VERSION_GE(NT_VERSION_VISTA) ? (PCWSTR)(LDR_PATH_IS_FLAGS | LDR_PATH_SEARCH_SYSTEM32) : NULL;

_Try_Load:
    Status = LdrLoadDll(DllPath, DllCharacteristics, DllName, DllHandle);
    if (NT_SUCCESS(Status))
    {
        return Status;
    }
    if (DllPath != NULL && Status == STATUS_INVALID_PARAMETER)
    {
        DllPath = NULL;
        goto _Try_Load;
    }
    return Status;
}

FORCEINLINE
NTSTATUS
PS_GetProcAddress(
    _In_ PVOID DllBase,
    _In_ PCANSI_STRING Name,
    _Out_ PVOID* Address)
{
    PANSI_STRING ProcName;
    ULONG ProcOridinal;

    if ((UINT_PTR)Name > MAXUSHORT)
    {
        ProcName = (PANSI_STRING)Name;
        ProcOridinal = 0;
    } else
    {
        ProcName = NULL;
        ProcOridinal = (ULONG)(ULONG_PTR)Name;
    }
    return LdrGetProcedureAddress(DllBase, ProcName, ProcOridinal, Address);
}

FORCEINLINE
_Success_(return > 0)
ULONG
PS_GetDirectory(
    _Out_writes_(Count) PWSTR Buffer,
    _In_ ULONG Count)
{
    PUNICODE_STRING ImagePathName = &NtCurrentPeb()->ProcessParameters->ImagePathName;
    USHORT i = ImagePathName->Length / sizeof(WCHAR);
    PWCH p = ImagePathName->Buffer;

    while (i > 0 && i <= Count)
    {
        if (p[--i] == OBJ_NAME_PATH_SEPARATOR)
        {
            memcpy(Buffer, p, i * sizeof(WCHAR));
            Buffer[i] = UNICODE_NULL;
            return i;
        }
    }
    return 0;
}

EXTERN_C_END
