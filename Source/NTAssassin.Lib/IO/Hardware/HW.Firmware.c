#include "NTAssassin.Lib.inl"

_Ret_maybenull_
PSYSTEM_FIRMWARE_TABLE_INFORMATION NTAPI HW_GetFirmwareTable(
    ULONG ProviderSignature,
    ULONG TableID,
    SYSTEM_FIRMWARE_TABLE_ACTION Action)
{
    NTSTATUS Status;
    ULONG Length;
    SYSTEM_FIRMWARE_TABLE_INFORMATION FirmwareInfo;
    PSYSTEM_FIRMWARE_TABLE_INFORMATION Buffer;

    FirmwareInfo.ProviderSignature = ProviderSignature;
    FirmwareInfo.Action = Action;
    FirmwareInfo.TableID = TableID;
    FirmwareInfo.TableBufferLength = 0;

    Status = NtQuerySystemInformation(SystemFirmwareTableInformation, &FirmwareInfo, sizeof(FirmwareInfo), &Length);
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        NtSetLastStatus(Status);
        return NULL;
    }

    _Analysis_assume_(Length >= sizeof(FirmwareInfo));
    Buffer = RtlAllocateHeap(NtGetProcessHeap(), 0, Length);
    if (Buffer == NULL)
    {
        return NULL;
    }

    RtlCopyMemory(Buffer, &FirmwareInfo, sizeof(FirmwareInfo));
    Status = NtQuerySystemInformation(SystemFirmwareTableInformation, Buffer, Length, &Length);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(NtGetProcessHeap(), 0, Buffer);
        return NULL;
    }

    return Buffer;
}
