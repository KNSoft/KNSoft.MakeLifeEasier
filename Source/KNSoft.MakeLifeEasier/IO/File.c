#include "../MakeLifeEasier.inl"

#pragma region Directory

NTSTATUS
NTAPI
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

#define FILE_FIND_BUFFER_SIZE PAGE_SIZE

NTSTATUS
NTAPI
IO_FindFile(
    _In_ HANDLE DirectoryHandle,
    _Out_ PVOID Buffer,
    _In_ ULONG BufferSize,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _In_opt_ PCUNICODE_STRING SearchFilter,
    _In_ BOOLEAN RestartScan,
    _Out_ PBOOL HasData)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    Status = NtQueryDirectoryFile(DirectoryHandle,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &IoStatusBlock,
                                  Buffer,
                                  BufferSize,
                                  FileInformationClass,
                                  FALSE,
                                  RTL_CONST_CAST(PUNICODE_STRING)(SearchFilter),
                                  RestartScan);
    if (Status == STATUS_NO_MORE_FILES)
    {
        *HasData = FALSE;
        Status = STATUS_SUCCESS;
    } else if (Status == STATUS_BUFFER_OVERFLOW || NT_SUCCESS(Status))
    {
        *HasData = TRUE;
        Status = STATUS_SUCCESS;
    }

    return Status;
}

NTSTATUS
NTAPI
IO_BeginFindFile(
    _Out_ PFILE_FIND FindData,
    _In_ HANDLE DirectoryHandle,
    _In_opt_ PCUNICODE_STRING SearchFilter,
    _In_ FILE_INFORMATION_CLASS FileInformationClass)
{
    NTSTATUS Status;
    PVOID Buffer;
    BOOL HasData;

    /* Allocate buffer */
    Status = Mem_Alloc(&Buffer, FILE_FIND_BUFFER_SIZE);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Find the first time */
    Status = IO_FindFile(DirectoryHandle,
                         Buffer,
                         FILE_FIND_BUFFER_SIZE,
                         FileInformationClass,
                         SearchFilter,
                         TRUE,
                         &HasData);

    /* Assign input values if success, or cleanup and exit if fail */
    if (NT_SUCCESS(Status))
    {
        FindData->SearchFilter = SearchFilter;
        FindData->FileInformationClass = FileInformationClass;
        FindData->DirectoryHandle = DirectoryHandle;
        FindData->Buffer = Buffer;
        FindData->Length = FILE_FIND_BUFFER_SIZE;
        FindData->HasData = HasData;
    } else
    {
        Mem_Free(Buffer);
    }
    return Status;
}

NTSTATUS
NTAPI
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

LOGICAL
NTAPI
IO_EndFindFile(
    _In_ PFILE_FIND FindData)
{
    return Mem_Free(FindData->Buffer);
}

#pragma endregion
