#pragma once

#include <KNSoft/NDK/NDK.h>

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
    } else
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
    } else
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
