#pragma once

#include "../MakeLifeEasier.h"

#include "../NT/Object.h"

EXTERN_C_START

#pragma region Directory

FORCEINLINE
NTSTATUS
IO_OpenDirectory(
    _Out_ PHANDLE DirectoryHandle,
    _In_ PUNICODE_STRING DirectoryPath,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess)
{
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(DirectoryPath, OBJ_CASE_INSENSITIVE);

    return NtOpenFile(DirectoryHandle,
                      DesiredAccess,
                      &ObjectAttributes,
                      &IoStatusBlock,
                      ShareAccess,
                      FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
}

#pragma endregion

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

FORCEINLINE
NTSTATUS
IO_ContinueFindFileFind(
    _Inout_ PFILE_FIND FindData)
{
    return IO_FindFile(FindData->DirectoryHandle,
                       FindData->Buffer,
                       FindData->Length,
                       FindData->FileInformationClass,
                       FindData->SearchFilter,
                       FALSE,
                       &FindData->HasData);
}

FORCEINLINE
_Success_(return != FALSE)
LOGICAL
IO_EndFindFile(
    _In_ PFILE_FIND FindData)
{
    return Mem_Free(FindData->Buffer);
}

#pragma endregion


#pragma region File Operations

FORCEINLINE
NTSTATUS
IO_CreateFile(
    _Out_ PHANDLE FileHandle,
    _In_ PUNICODE_STRING FileName,
    _In_opt_ HANDLE RootDirectory,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions)
{
    OBJECT_ATTRIBUTES Object;
    IO_STATUS_BLOCK IoStatusBlock;

    NT_InitObject(&Object, FileName, OBJ_CASE_INSENSITIVE, RootDirectory);
    return NtCreateFile(FileHandle,
                        DesiredAccess,
                        &Object,
                        &IoStatusBlock,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        ShareAccess,
                        CreateDisposition,
                        CreateOptions,
                        NULL,
                        0);
}

FORCEINLINE
NTSTATUS
IO_CreateWin32File(
    _Out_ PHANDLE FileHandle,
    _In_ PCWSTR FileName,
    _In_opt_ HANDLE RootDirectory,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Object;
    UNICODE_STRING NtPath;
    IO_STATUS_BLOCK IoStatusBlock;

    Status = NT_InitWin32PathObject(&Object, FileName, RootDirectory, &NtPath);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    Status = NtCreateFile(FileHandle,
                          DesiredAccess,
                          &Object,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          ShareAccess,
                          CreateDisposition,
                          CreateOptions,
                          NULL,
                          0);
    NT_FreeNtPath(&NtPath);
    return Status;
}

FORCEINLINE
NTSTATUS
IO_OpenFile(
    _Out_ PHANDLE FileHandle,
    _In_ PUNICODE_STRING FileName,
    _In_opt_ HANDLE RootDirectory,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess)
{

    OBJECT_ATTRIBUTES Object;
    IO_STATUS_BLOCK IoStatusBlock;

    NT_InitObject(&Object, FileName, OBJ_CASE_INSENSITIVE, RootDirectory);
    return NtOpenFile(FileHandle,
                      DesiredAccess,
                      &Object,
                      &IoStatusBlock,
                      ShareAccess,
                      FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
}

FORCEINLINE
NTSTATUS
IO_OpenWin32File(
    _Out_ PHANDLE FileHandle,
    _In_ PCWSTR FileName,
    _In_opt_ HANDLE RootDirectory,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Object;
    UNICODE_STRING NtPath;
    IO_STATUS_BLOCK IoStatusBlock;

    Status = NT_InitWin32PathObject(&Object, FileName, RootDirectory, &NtPath);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    Status = NtOpenFile(FileHandle,
                        DesiredAccess,
                        &Object,
                        &IoStatusBlock,
                        ShareAccess,
                        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    NT_FreeNtPath(&NtPath);
    return Status;
}

FORCEINLINE
NTSTATUS
IO_DeleteFile(
    _In_ PUNICODE_STRING FileName,
    _In_opt_ HANDLE RootDirectory)
{
    NTSTATUS Status;
    HANDLE FileHandle;

    Status = IO_CreateFile(&FileHandle,
                           FileName,
                           RootDirectory,
                           DELETE | SYNCHRONIZE,
                           0,
                           FILE_OPEN,
                           FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_DELETE_ON_CLOSE);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    return NtClose(FileHandle);
}

FORCEINLINE
NTSTATUS
IO_DeleteWin32File(
    _In_ PCWSTR FileName,
    _In_opt_ HANDLE RootDirectory)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Object;
    UNICODE_STRING NtPath;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;

    Status = NT_InitWin32PathObject(&Object, FileName, RootDirectory, &NtPath);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = NtCreateFile(&FileHandle,
                          DELETE | SYNCHRONIZE,
                          &Object,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_OPEN,
                          FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_DELETE_ON_CLOSE,
                          NULL,
                          0);
    NT_FreeNtPath(&NtPath);
    return NtClose(FileHandle);
}

FORCEINLINE
NTSTATUS
IO_GetFileAttributes(
    _In_ PUNICODE_STRING FileName,
    _In_opt_ HANDLE RootDirectory,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION Attributes)
{
    OBJECT_ATTRIBUTES Object;

    NT_InitObject(&Object, FileName, OBJ_CASE_INSENSITIVE, RootDirectory);
    return NtQueryFullAttributesFile(&Object, Attributes);
}

FORCEINLINE
NTSTATUS
IO_GetWin32FileAttributes(
    _In_ PCWSTR FileName,
    _In_opt_ HANDLE RootDirectory,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION Attributes)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Object;
    UNICODE_STRING NtPath;

    NT_InitWin32PathObject(&Object, FileName, RootDirectory, &NtPath);
    Status = NtQueryFullAttributesFile(&Object, Attributes);
    NT_FreeNtPath(&NtPath);
    return Status;
}

FORCEINLINE
NTSTATUS
IO_IsDirectory(
    _In_ PUNICODE_STRING FileName,
    _In_opt_ HANDLE RootDirectory,
    _Out_ PBOOLEAN IsDirectory)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Object;
    FILE_NETWORK_OPEN_INFORMATION Attributes;

    NT_InitObject(&Object, FileName, OBJ_CASE_INSENSITIVE, RootDirectory);
    Status = NtQueryFullAttributesFile(&Object, &Attributes);
    if (NT_SUCCESS(Status))
    {
        *IsDirectory = BooleanFlagOn(Attributes.FileAttributes, FILE_ATTRIBUTE_DIRECTORY);
    }
    return Status;
}

FORCEINLINE
NTSTATUS
IO_ReadFile(
    _In_ HANDLE FileHandle,
    _In_opt_ PLARGE_INTEGER Offset,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length)
{
    IO_STATUS_BLOCK IoStatusBlock;

    return NtReadFile(FileHandle, NULL, NULL, NULL, &IoStatusBlock, Buffer, Length, Offset, NULL);
}

FORCEINLINE
NTSTATUS
IO_WriteFile(
    _In_ HANDLE FileHandle,
    _In_opt_ PLARGE_INTEGER Offset,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length)
{
    IO_STATUS_BLOCK IoStatusBlock;

    return NtWriteFile(FileHandle, NULL, NULL, NULL, &IoStatusBlock, Buffer, Length, Offset, NULL);
}

FORCEINLINE
NTSTATUS
IO_GetFileSize(
    _In_ HANDLE FileHandle,
    _Out_ PULONGLONG Size)
{
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION Info;
    NTSTATUS Status;

    Status = NtQueryInformationFile(FileHandle, &IoStatusBlock, &Info, sizeof(Info), FileStandardInformation);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    *Size = (ULONGLONG)Info.EndOfFile.QuadPart;
    return STATUS_SUCCESS;
}

FORCEINLINE
NTSTATUS
IO_SetFileSize(
    _In_ HANDLE FileHandle,
    _In_ ULONGLONG Size)
{
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_END_OF_FILE_INFORMATION Info;

    Info.EndOfFile.QuadPart = Size;
    return NtSetInformationFile(FileHandle, &IoStatusBlock, &Info, sizeof(Info), FileEndOfFileInformation);
}

FORCEINLINE
NTSTATUS
IO_DisposeFile(
    _In_ HANDLE FileHandle)
{
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_DISPOSITION_INFORMATION Info = { TRUE };

    return NtSetInformationFile(FileHandle, &IoStatusBlock, &Info, sizeof(Info), FileDispositionInformation);
}

#pragma endregion

EXTERN_C_END
