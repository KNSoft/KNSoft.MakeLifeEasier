#include "../MakeLifeEasier.inl"

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
    Buffer = Mem_Alloc(FILE_FIND_BUFFER_SIZE);
    if (Buffer == NULL)
    {
        return STATUS_NO_MEMORY;
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

#pragma endregion

