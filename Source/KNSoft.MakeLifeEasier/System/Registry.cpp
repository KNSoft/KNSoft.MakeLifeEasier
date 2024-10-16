﻿#include "../MakeLifeEasier.inl"

NTSTATUS
NTAPI
Sys_RegOpenKeyEx(
    _Out_ PHANDLE KeyHandle,
    _In_opt_ HANDLE RootKey,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCUNICODE_STRING Path)
{
    OBJECT_ATTRIBUTES ObjectAttributes;

    InitializeObjectAttributes(&ObjectAttributes,
                               (PUNICODE_STRING)Path,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               RootKey,
                               NULL);

    return NtOpenKey(KeyHandle, DesiredAccess, &ObjectAttributes);
}

template <typename T>
static
NTSTATUS
Sys_RegQueryNumericValue_Impl(
    _In_ HANDLE KeyHandle,
    _In_ PCUNICODE_STRING ValueName,
    _In_ ULONG Type,
    _Out_ T* Value)
{
    NTSTATUS Status;
    ULONG Length;
    SYS_REG_DEFINE_VALUE_DATA(Info, sizeof(T));

    Status = NtQueryValueKey(KeyHandle,
                             (PUNICODE_STRING)ValueName,
                             KeyValuePartialInformation,
                             &Info,
                             sizeof(Info),
                             &Length);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    } else if (Info.BaseType.Type != Type)
    {
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    *Value = *(T*)(Info.BaseType.Data);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
Sys_RegQueryDword(
    _In_ HANDLE KeyHandle,
    _In_ PCUNICODE_STRING ValueName,
    _Out_ PDWORD Value)
{
    return Sys_RegQueryNumericValue_Impl<DWORD>(KeyHandle, ValueName, REG_DWORD, Value);
}

NTSTATUS
NTAPI
Sys_RegQueryQword(
    _In_ HANDLE KeyHandle,
    _In_ PCUNICODE_STRING ValueName,
    _Out_ PQWORD Value)
{
    return Sys_RegQueryNumericValue_Impl<QWORD>(KeyHandle, ValueName, REG_QWORD, Value);
}

NTSTATUS
NTAPI
Sys_RegQueryData(
    _In_ HANDLE KeyHandle,
    _In_ PCUNICODE_STRING ValueName,
    _Out_ PKEY_VALUE_PARTIAL_INFORMATION* Data)
{
    NTSTATUS Status;
    ULONG Length;
    PKEY_VALUE_PARTIAL_INFORMATION Buffer;

    Status = NtQueryValueKey(KeyHandle,
                             (PUNICODE_STRING)ValueName,
                             KeyValuePartialInformation,
                             NULL,
                             0,
                             &Length);
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        return NT_SUCCESS(Status) ? STATUS_UNSUCCESSFUL : Status;
    }
    if (!NT_SUCCESS(Mem_Alloc((PVOID*)&Buffer, Length)))
    {
        return STATUS_NO_MEMORY;
    }
    Status = NtQueryValueKey(KeyHandle,
                             (PUNICODE_STRING)ValueName,
                             KeyValuePartialInformation,
                             Buffer,
                             Length,
                             &Length);
    if (!NT_SUCCESS(Status))
    {
        Mem_Free(Buffer);
        return Status;
    }

    *Data = Buffer;
    return STATUS_SUCCESS;
}
