#include "../MakeLifeEasier.inl"

#pragma region Pipe

NTSTATUS
NTAPI
IO_CreatePipe(
    _In_ HANDLE PipeDirectoryHandle,
    _Out_ PHANDLE Handle,
    _Out_ PHANDLE PeerHandle,
    _In_ ULONG Mode,
    _In_ ULONG BufferSize)
{
    NTSTATUS Status;
    HANDLE CreatedHandle, CreatedPeerHandle;
    ACCESS_MASK PipeAccess, PeerAccess;
    ULONG PipeShareAccess, PeerShareAccess;
    UNICODE_STRING Name = { 0 };
    OBJECT_ATTRIBUTES ObjectAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(&Name, OBJ_CASE_INSENSITIVE);
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER DefaultTimeout;

    switch (Mode)
    {
        case FILE_PIPE_INBOUND:
            PipeAccess = GENERIC_READ | SYNCHRONIZE | FILE_WRITE_ATTRIBUTES;
            PeerAccess = GENERIC_WRITE | SYNCHRONIZE | FILE_READ_ATTRIBUTES;
            PipeShareAccess = FILE_SHARE_WRITE;
            PeerShareAccess = FILE_SHARE_READ;
            break;

        case FILE_PIPE_OUTBOUND:
            PipeAccess = GENERIC_WRITE | SYNCHRONIZE | FILE_READ_ATTRIBUTES;
            PeerAccess = GENERIC_READ | SYNCHRONIZE | FILE_WRITE_ATTRIBUTES;
            PipeShareAccess = FILE_SHARE_READ;
            PeerShareAccess = FILE_SHARE_WRITE;
            break;

        case FILE_PIPE_FULL_DUPLEX:
            PipeAccess = PeerAccess = GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE;
            PipeShareAccess = PeerShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE;
            break;

        default:
            return STATUS_INVALID_PARAMETER;
    }

    ObjectAttributes.RootDirectory = PipeDirectoryHandle;
    DefaultTimeout.QuadPart = -10000000LL;
    Status = NtCreateNamedPipeFile(&CreatedHandle,
                                   PipeAccess,
                                   &ObjectAttributes,
                                   &IoStatusBlock,
                                   PipeShareAccess,
                                   FILE_CREATE,
                                   0,
                                   FILE_PIPE_BYTE_STREAM_TYPE,
                                   FILE_PIPE_BYTE_STREAM_MODE,
                                   FILE_PIPE_QUEUE_OPERATION,
                                   1,
                                   BufferSize,
                                   BufferSize,
                                   &DefaultTimeout);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ObjectAttributes.RootDirectory = CreatedHandle;
    Status = NtCreateFile(&CreatedPeerHandle,
                          PeerAccess,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          0,
                          PeerShareAccess,
                          FILE_OPEN,
                          FILE_NON_DIRECTORY_FILE,
                          NULL,
                          0);
    if (NT_SUCCESS(Status))
    {
        *Handle = CreatedHandle;
        *PeerHandle = CreatedPeerHandle;
    } else
    {
        NtClose(CreatedHandle);
    }
    return Status;
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

#pragma region File Map

NTSTATUS
NTAPI
IO_MapFileEx(
    _In_ HANDLE FileHandle,
    _In_ ULONG AllocationAttributes,
    _In_ ULONG PageProtection,
    _Out_ PIO_FILE_MAP MapInfo)
{
    NTSTATUS Status;
    ULONGLONG FileSize;
    HANDLE SectionHandle;
    PVOID BaseAddress;
    SIZE_T PageSize;
    
    Status = IO_GetFileSize(FileHandle, &FileSize);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    } else if (FileSize == 0)
    {
        return STATUS_MAPPED_FILE_SIZE_ZERO;
    }
#ifndef _WIN64
    if (FileSize > MAXSIZE_T - PAGE_SIZE + 1)
    {
        return STATUS_NO_MEMORY;
    }
#endif

    Status = NtCreateSection(&SectionHandle,
                             SECTION_ALL_ACCESS,
                             NULL,
                             NULL,
                             PageProtection,
                             AllocationAttributes,
                             FileHandle);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    BaseAddress = NULL;
    PageSize = (SIZE_T)FileSize;
    Status = NtMapViewOfSection(SectionHandle,
                                NtCurrentProcess(),
                                &BaseAddress,
                                0,
                                0,
                                NULL,
                                &PageSize,
                                ViewUnmap,
                                0,
                                PageProtection);
    if (NT_SUCCESS(Status))
    {
        MapInfo->FileSize = (SIZE_T)FileSize;
        MapInfo->PageSize = PageSize;
        MapInfo->SectionHandle = SectionHandle;
        MapInfo->BaseAddress = BaseAddress;
    } else
    {
        NtClose(SectionHandle);
    }
    return Status;
}

#pragma endregion
