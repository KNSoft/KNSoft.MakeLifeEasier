#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

FORCEINLINE
NTSTATUS
PS_GetProcAddress(
    _In_ PVOID DllBase,
    _In_ PCANSI_STRING Name,
    _Out_ PVOID* Address)
{
    PANSI_STRING ProcName;
    ULONG ProcOridinal;

    if ((UINT_PTR)Name > MAXWORD)
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
    PUNICODE_STRING str = &NtCurrentPeb()->ProcessParameters->ImagePathName;
    USHORT i = str->Length / sizeof(WCHAR);
    PWCH p = str->Buffer;

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
