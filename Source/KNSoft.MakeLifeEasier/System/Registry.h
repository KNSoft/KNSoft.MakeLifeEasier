#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

FORCEINLINE
NTSTATUS
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

FORCEINLINE
NTSTATUS
Sys_RegOpenKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCUNICODE_STRING Path)
{
    return Sys_RegOpenKeyEx(KeyHandle, NULL, DesiredAccess, Path);
}

FORCEINLINE
NTSTATUS
Sys_RegCreateKeyEx(
    _Out_ PHANDLE KeyHandle,
    _In_opt_ HANDLE RootKey,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCUNICODE_STRING Path,
    _In_ ULONG CreateOptions,
    _Out_opt_ PULONG Disposition)
{
    OBJECT_ATTRIBUTES ObjectAttributes;

    InitializeObjectAttributes(&ObjectAttributes,
                               (PUNICODE_STRING)Path,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               RootKey,
                               NULL);

    return NtCreateKey(KeyHandle, DesiredAccess, &ObjectAttributes, 0, NULL, CreateOptions, Disposition);
}

FORCEINLINE
NTSTATUS
Sys_RegCreateKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCUNICODE_STRING Path)
{
    return Sys_RegCreateKeyEx(KeyHandle, NULL, DesiredAccess, Path, REG_OPTION_NON_VOLATILE, NULL);
}

FORCEINLINE
NTSTATUS
Sys_RegSetData(
    _In_ HANDLE KeyHandle,
    _In_ PCUNICODE_STRING ValueName,
    _In_ ULONG Type,
    _In_reads_bytes_(DataSize) LPCVOID Data,
    _In_ ULONG DataSize)
{
    return NtSetValueKey(KeyHandle, (PUNICODE_STRING)ValueName, 0, Type, (PVOID)Data, DataSize);
}

FORCEINLINE
NTSTATUS
Sys_RegSetDword(
    _In_ HANDLE KeyHandle,
    _In_ PCUNICODE_STRING ValueName,
    _In_ DWORD Value)
{
    return Sys_RegSetData(KeyHandle, ValueName, REG_DWORD, &Value, sizeof(Value));
}

#define SYS_REG_DEFINE_VALUE_DATA(VarName, ValueSize) DEFINE_ANYSIZE_STRUCT(VarName, KEY_VALUE_PARTIAL_INFORMATION, UCHAR, ValueSize)
#define SYS_REG_VALUE_DATA_SIZE(ValueSize) (UFIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + (ValueSize))

MLE_API
NTSTATUS
NTAPI
Sys_RegQueryDword(
    _In_ HANDLE KeyHandle,
    _In_ PCUNICODE_STRING ValueName,
    _Out_ PDWORD Value);

MLE_API
NTSTATUS
NTAPI
Sys_RegQueryQword(
    _In_ HANDLE KeyHandle,
    _In_ PCUNICODE_STRING ValueName,
    _Out_ PQWORD Value);

MLE_API
NTSTATUS
NTAPI
Sys_RegQueryData(
    _In_ HANDLE KeyHandle,
    _In_ PCUNICODE_STRING ValueName,
    _Out_ PKEY_VALUE_PARTIAL_INFORMATION* Data);

EXTERN_C_END
