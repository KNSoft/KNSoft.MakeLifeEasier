#include "NTAssassin.Lib.inl"

#define FILE_FIND_BUFFER_SIZE PAGE_SIZE

NTSTATUS NTAPI File_Find(
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

NTSTATUS NTAPI File_BeginFind(
    _Out_ PFILE_FIND FindData,
    _In_ HANDLE DirectoryHandle,
    _In_opt_ PCUNICODE_STRING SearchFilter,
    _In_ FILE_INFORMATION_CLASS FileInformationClass)
{
    NTSTATUS Status;
    PVOID Buffer;
    BOOL HasData;

    /* Allocate buffer */
    Status = NTA_Alloc(&Buffer, FILE_FIND_BUFFER_SIZE);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Find the first time */
    Status = File_Find(DirectoryHandle,
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
        NTA_Free(Buffer);
    }
    return Status;
}

NTSTATUS NTAPI File_ContinueFind(_Inout_ PFILE_FIND FindData)
{
    return File_Find(FindData->DirectoryHandle,
                     FindData->Buffer,
                     FindData->Length,
                     FindData->FileInformationClass,
                     FindData->SearchFilter,
                     FALSE,
                     &FindData->HasData);
}

VOID NTAPI File_EndFind(_In_ PFILE_FIND FindData)
{
    NTA_Free(FindData->Buffer);
}
