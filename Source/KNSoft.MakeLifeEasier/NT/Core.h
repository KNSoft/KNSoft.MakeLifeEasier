#pragma once

#include <KNSoft/NDK/NDK.h>

#include "../Memory/Core.h"

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

FORCEINLINE
PUNICODE_STRING
NT_AllocStringW(
    _In_ USHORT CchLength)
{
    PUNICODE_STRING p;

    p = (PUNICODE_STRING)Mem_Alloc(sizeof(UNICODE_STRING) + CchLength * sizeof(WCHAR) + sizeof(UNICODE_NULL));
    if (p == NULL)
    {
        return p;
    }
    p->Length = CchLength * sizeof(WCHAR);
    p->MaximumLength = p->Length + sizeof(UNICODE_NULL);
    p->Buffer = (PWCH)Add2Ptr(p, sizeof(UNICODE_STRING));

    return p;
}

FORCEINLINE
PANSI_STRING
NT_AllocStringA(
    _In_ USHORT CchLength)
{
    PANSI_STRING p;

    p = (PANSI_STRING)Mem_Alloc(sizeof(ANSI_STRING) + CchLength * sizeof(CHAR) + sizeof(ANSI_NULL));
    if (p == NULL)
    {
        return p;
    }
    p->Length = CchLength * sizeof(CHAR);
    p->MaximumLength = p->Length + sizeof(ANSI_NULL);
    p->Buffer = (PCHAR)Add2Ptr(p, sizeof(ANSI_STRING));

    return p;
}

FORCEINLINE
_Success_(return != FALSE)
LOGICAL
NT_FreeStringW(
    __drv_freesMem(Mem) _Frees_ptr_ _Post_invalid_ PUNICODE_STRING String)
{
    return Mem_Free(String);
}

FORCEINLINE
_Success_(return != FALSE)
LOGICAL
NT_FreeStringA(
    __drv_freesMem(Mem) _Frees_ptr_ _Post_invalid_ PANSI_STRING String)
{
    return Mem_Free(String);
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

#pragma region Time

_Ret_maybenull_
FORCEINLINE
PLARGE_INTEGER
NT_MillisecondsToTimeout(
    _Out_ CONST PLARGE_INTEGER Timeout,
    _In_ ULONG Milliseconds)
{
    if (Milliseconds == INFINITE)
    {
        Timeout->QuadPart = MINLONGLONG;
        return NULL;
    } else
    {
        Timeout->QuadPart = Milliseconds * -10000LL; // ms to 100ns
        return Timeout;
    }
}

#pragma endregion
