#include "NTAssassin.Lib.inl"

NTSTATUS NTAPI Device_QueryStorageProperty(
    _In_ HANDLE DeviceHandle,
    _In_ PSTORAGE_PROPERTY_QUERY Query,
    _In_ ULONG QuerySize,
    _Out_ _At_(*StoragePropertyLength,
               _Readable_bytes_(*StoragePropertyLength)
               _Writable_bytes_(*StoragePropertyLength)
               _Post_readable_byte_size_(*StoragePropertyLength)) PVOID* StorageProperty,
    _Out_opt_ PULONG StoragePropertyLength)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    STORAGE_DESCRIPTOR_HEADER sdh;
    PVOID Buffer;

    /* Query header for size */
    Status = NtDeviceIoControlFile(DeviceHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_STORAGE_QUERY_PROPERTY,
                                   Query,
                                   QuerySize,
                                   &sdh,
                                   sizeof(sdh));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Allocate buffer and receive data */
    Status = NTA_Alloc(&Buffer, sdh.Size);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    Status = NtDeviceIoControlFile(DeviceHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_STORAGE_QUERY_PROPERTY,
                                   Query,
                                   QuerySize,
                                   Buffer,
                                   sdh.Size);
    if (NT_SUCCESS(Status))
    {
        *StorageProperty = Buffer;
        if (StoragePropertyLength != NULL)
        {
            *StoragePropertyLength = sdh.Size;
        }
    } else
    {
        NTA_Free(Buffer);
    }
    return Status;
}
