﻿#include "../MakeLifeEasier.inl"

#pragma region Basic

NTSTATUS
NTAPI
IO_OpenDevice(
    _In_ PCUNICODE_STRING DeviceName,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE DeviceHandle)
{
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttribute = RTL_CONSTANT_OBJECT_ATTRIBUTES(DeviceName, OBJ_CASE_INSENSITIVE);

    return NtOpenFile(DeviceHandle,
                      DesiredAccess,
                      &ObjectAttribute,
                      &IoStatusBlock,
                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                      FILE_NON_DIRECTORY_FILE);
}

#pragma endregion

#pragma region Storage

NTSTATUS
NTAPI
IO_QueryStorageProperty(
    _In_ HANDLE DeviceHandle,
    _In_ PSTORAGE_PROPERTY_QUERY Query,
    _In_ ULONG QuerySize,
    _Out_ _At_(*StorageProperty,
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
    Status = Mem_Alloc(&Buffer, sdh.Size);
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
        Mem_Free(Buffer);
    }
    return Status;
}

#pragma endregion

#pragma region Firmware

NTSTATUS
NTAPI
IO_GetFirmwareTable(
    _In_ ULONG ProviderSignature,
    _In_ ULONG TableID,
    _In_ SYSTEM_FIRMWARE_TABLE_ACTION Action,
    _Out_ _At_(*FirmwareInformation,
               _Maybenull_
               _Readable_bytes_(*FirmwareInformationLength)
               _Writable_bytes_(*FirmwareInformationLength)
               _Post_readable_byte_size_(*FirmwareInformationLength)) PSYSTEM_FIRMWARE_TABLE_INFORMATION* FirmwareInformation,
    _Out_opt_ PULONG FirmwareInformationLength)
{
    NTSTATUS Status;
    PSYSTEM_FIRMWARE_TABLE_INFORMATION Buffer;
    ULONG Length;
    SYSTEM_FIRMWARE_TABLE_INFORMATION FirmwareInfo = {
        .ProviderSignature = ProviderSignature,
        .Action = Action,
        .TableID = TableID,
        .TableBufferLength = 0
    };

    Status = NtQuerySystemInformation(SystemFirmwareTableInformation, &FirmwareInfo, sizeof(FirmwareInfo), &Length);
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        if (NT_SUCCESS(Status))
        {
            *FirmwareInformation = NULL;
            if (FirmwareInformationLength != NULL)
            {
                *FirmwareInformationLength = 0;
            }
        }
        return Status;
    }

    _Analysis_assume_(Length >= sizeof(FirmwareInfo));
    Status = Mem_Alloc(&Buffer, Length);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    RtlCopyMemory(Buffer, &FirmwareInfo, sizeof(FirmwareInfo));
    Status = NtQuerySystemInformation(SystemFirmwareTableInformation, Buffer, Length, &Length);
    if (!NT_SUCCESS(Status))
    {
        Mem_Free(Buffer);
    } else
    {
        *FirmwareInformation = Buffer;
        if (FirmwareInformationLength != NULL)
        {
            *FirmwareInformationLength = Length;
        }
    }
    return Status;
}

#pragma endregion
