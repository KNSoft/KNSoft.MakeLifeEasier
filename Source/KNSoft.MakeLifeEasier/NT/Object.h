#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

MLE_API EXTERN_C CONST OBJECT_ATTRIBUTES NT_EmptyObjectAttribute;

#pragma region NT String

/*
 * If failed, the caller could use
 * STATUS_BUFFER_TOO_SMALL, ERROR_INSUFFICIENT_BUFFER, ... as error code
 */

FORCEINLINE
_Success_(return != FALSE)
LOGICAL
NT_CopyStringW(
    _In_ PCUNICODE_STRING NtString,
    _Out_writes_(Count) PWSTR Buffer,
    _In_ ULONG Count)
{
    USHORT i;

    i = NtString->Length / sizeof(WCHAR);
    if (Count <= i)
    {
        return FALSE;
    }
    memcpy(Buffer, NtString->Buffer, NtString->Length);
    Buffer[i] = UNICODE_NULL;
    return TRUE;
}

FORCEINLINE
_Success_(return != FALSE)
LOGICAL
NT_CopyStringA(
    _In_ PCANSI_STRING NtString,
    _Out_writes_(Count) PSTR Buffer,
    _In_ ULONG Count)
{
    USHORT i;

    i = NtString->Length / sizeof(CHAR);
    if (Count <= i)
    {
        return FALSE;
    }
    memcpy(Buffer, NtString->Buffer, NtString->Length);
    Buffer[i] = ANSI_NULL;
    return TRUE;
}

#pragma endregion

#pragma region Path

/* NtPath should be freed by NT_FreeNtPath */
FORCEINLINE
NTSTATUS
NT_Win32PathToNtPath(
    _In_ PCWSTR Win32Path,
    _In_opt_ HANDLE RootDirectory,
    _Out_ PUNICODE_STRING NtPath)
{
    return RtlDosPathNameToNtPathName_U_WithStatus(Win32Path, NtPath, NULL, NULL);
}

/* NtPath should be freed by NT_FreeNtPath */
FORCEINLINE
NTSTATUS
NT_InitWin32PathObject(
    _Out_ POBJECT_ATTRIBUTES Object,
    _In_ PCWSTR Win32Path,
    _In_opt_ HANDLE RootDirectory,
    _Out_ PUNICODE_STRING NtPath)
{
    NTSTATUS Status;

    Status = NT_Win32PathToNtPath(Win32Path, RootDirectory, NtPath);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    NT_InitObject(Object, NtPath, OBJ_CASE_INSENSITIVE, RootDirectory);
    return STATUS_SUCCESS;
}

FORCEINLINE
LOGICAL
NT_FreeNtPath(
    _In_ PUNICODE_STRING NtPath)
{
    return RtlFreeHeap(RtlProcessHeap(), 0, NtPath->Buffer);
}

#pragma endregion

EXTERN_C_END
