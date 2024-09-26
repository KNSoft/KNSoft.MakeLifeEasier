#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

#pragma region Directory

MLE_API
NTSTATUS
NTAPI
IO_OpenDirectory(
    _Out_ PHANDLE DirectoryHandle,
    _In_ PUNICODE_STRING DirectoryPath,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess);

#pragma region Find File

typedef struct _FILE_FIND
{
    /* Initialize parameters */
    PCUNICODE_STRING SearchFilter;
    FILE_INFORMATION_CLASS FileInformationClass;
    HANDLE DirectoryHandle;

    /* Output data */
    BOOL HasData;
    DECLSPEC_ALIGN(4) PVOID Buffer;
    ULONG Length;
} FILE_FIND, *PFILE_FIND;

MLE_API
NTSTATUS
NTAPI
IO_FindFile(
    _In_ HANDLE DirectoryHandle,
    _Out_ PVOID Buffer,
    _In_ ULONG BufferSize,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _In_opt_ PCUNICODE_STRING SearchFilter,
    _In_ BOOLEAN RestartScan,
    _Out_ PBOOL HasData);

MLE_API
NTSTATUS
NTAPI
IO_BeginFindFile(
    _Out_ PFILE_FIND FindData,
    _In_ HANDLE DirectoryHandle,
    _In_opt_ PCUNICODE_STRING SearchFilter,
    _In_ FILE_INFORMATION_CLASS FileInformationClass);

MLE_API
NTSTATUS
NTAPI
IO_ContinueFindFileFind(
    _Inout_ PFILE_FIND FindData);

MLE_API
LOGICAL
NTAPI
IO_EndFindFile(
    _In_ PFILE_FIND FindData);

#pragma endregion

#pragma endregion

EXTERN_C_END
