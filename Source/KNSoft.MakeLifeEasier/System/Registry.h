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
