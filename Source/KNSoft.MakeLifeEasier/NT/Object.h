#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

#pragma region Path

/* See also InitializeObjectAttributes */

FORCEINLINE
VOID
NT_InitObject(
    _Out_ POBJECT_ATTRIBUTES Object,
    _In_opt_ PUNICODE_STRING ObjectName,
    _In_opt_ ULONG Attributes,
    _In_opt_ HANDLE RootDirectory)
{
    InitializeObjectAttributes(Object, ObjectName, Attributes, RootDirectory, NULL);
}

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
}

FORCEINLINE
LOGICAL
NT_FreeNtPath(
    _In_ PUNICODE_STRING NtPath)
{
    return RtlFreeHeap(NtGetProcessHeap(), 0, NtPath->Buffer);
}

#pragma endregion

EXTERN_C_END
