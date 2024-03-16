#include "NTAssassin.Lib.inl"

NTSTATUS NTAPI Device_QueryStorageProperty(
    _In_ HANDLE DeviceHandle,
    _In_ PSTORAGE_PROPERTY_QUERY Query,
    _In_ ULONG QuerySize,
    _Out_ PVOID* AllocateBuffer)
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
    Buffer = RtlAllocateHeap(NtGetProcessHeap(), 0, sdh.Size);
    if (Buffer == NULL)
    {
        return STATUS_NO_MEMORY;
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
        *AllocateBuffer = Buffer;
    } else
    {
        RtlFreeHeap(NtGetProcessHeap(), 0, Buffer);
    }
    return Status;
}

VOID NTAPI Device_FreeStorageProperty(_Frees_ptr_ PVOID Buffer)
{
    RtlFreeHeap(NtGetProcessHeap(), 0, Buffer);
}
