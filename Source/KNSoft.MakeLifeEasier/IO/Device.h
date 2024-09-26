#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

#pragma region Basic

MLE_API
NTSTATUS
NTAPI
IO_OpenDevice(
    _In_ PCUNICODE_STRING DeviceName,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE DeviceHandle);

#pragma endregion

#pragma region Storage

MLE_API
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
    _Out_opt_ PULONG StoragePropertyLength);

#pragma endregion

#pragma region Firmware

MLE_API
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
    _Out_opt_ PULONG FirmwareInformationLength);

#pragma endregion

EXTERN_C_END
