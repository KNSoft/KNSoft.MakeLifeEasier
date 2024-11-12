#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

#pragma region String

/* See also RtlInitUnicodeStringEx */
FORCEINLINE
NTSTATUS
NT_InitStringW(
    _Out_ PUNICODE_STRING NTString,
    _In_ PWSTR String)
{
    SIZE_T Size;
 
    if (String != NULL)
    {
        Size = wcslen(String) * sizeof(WCHAR);
        if (Size > (MAXUSHORT & ~1) - sizeof(WCHAR))
        {
            return STATUS_NAME_TOO_LONG;
        }
        NTString->Length = (USHORT)Size;
        NTString->MaximumLength = (USHORT)Size + sizeof(WCHAR);
    }
    else
    {
        NTString->Length = NTString->MaximumLength = 0;
    }

    NTString->Buffer = (PWCHAR)String;
    return STATUS_SUCCESS;
}

/* See also RtlInitAnsiStringEx */
FORCEINLINE
NTSTATUS
NT_InitStringA(
    _Out_ PANSI_STRING NTString,
    _In_ PSTR String)
{
    SIZE_T Size;
 
    if (String != NULL)
    {
        Size = strlen(String);
        if (Size > (MAXUSHORT - sizeof(CHAR)))
        {
            return STATUS_NAME_TOO_LONG;
        }
        NTString->Length = (USHORT)Size;
        NTString->MaximumLength = (USHORT)Size + sizeof(CHAR);
    }
    else
    {
        NTString->Length = NTString->MaximumLength = 0;
    }
 
    NTString->Buffer = (PCHAR)String;
    return STATUS_SUCCESS;
}

#pragma endregion

#pragma region Object Attributes

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

FORCEINLINE
VOID
NT_InitEmptyObject(
    _Out_ POBJECT_ATTRIBUTES Object)
{
    InitializeObjectAttributes(Object, NULL, 0, NULL, NULL);
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
