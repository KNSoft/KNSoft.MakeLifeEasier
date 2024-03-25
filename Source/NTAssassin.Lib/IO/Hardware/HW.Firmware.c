#include "NTAssassin.Lib.inl"

NTSTATUS NTAPI HW_GetFirmwareTable(
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
    Status = NTA_Alloc(&Buffer, Length);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    RtlCopyMemory(Buffer, &FirmwareInfo, sizeof(FirmwareInfo));
    Status = NtQuerySystemInformation(SystemFirmwareTableInformation, Buffer, Length, &Length);
    if (!NT_SUCCESS(Status))
    {
        NTA_Free(Buffer);
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
