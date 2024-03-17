#include "NTAssassin.Lib.inl"

// TODO: Improve SAL
NTSTATUS NTAPI HW_GetFirmwareTable(
    _In_ ULONG ProviderSignature,
    _In_ ULONG TableID,
    _In_ SYSTEM_FIRMWARE_TABLE_ACTION Action,
    _Out_ PSYSTEM_FIRMWARE_TABLE_INFORMATION* FirmwareInformation,
    _Out_ PULONG FirmwareInformationLength)
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
            *FirmwareInformationLength = 0;
        }
        return Status;
    }

    _Analysis_assume_(Length >= sizeof(FirmwareInfo));
    Buffer = RtlAllocateHeap(NtGetProcessHeap(), 0, Length);
    if (Buffer == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    RtlCopyMemory(Buffer, &FirmwareInfo, sizeof(FirmwareInfo));
    Status = NtQuerySystemInformation(SystemFirmwareTableInformation, Buffer, Length, &Length);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(NtGetProcessHeap(), 0, Buffer);
    } else
    {
        *FirmwareInformation = Buffer;
        *FirmwareInformationLength = Length;
    }
    return Status;
}
