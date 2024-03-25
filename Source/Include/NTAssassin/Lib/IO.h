#pragma once

#include "../NDK/NT/MinDef.h"

#include <winioctl.h>

EXTERN_C_START

#pragma region Path

NTA_API
NTSTATUS NTAPI Path_ToNtPath(
    _In_z_ PCWSTR Path,
    _Out_ PUNICODE_STRING NtPath,
    _Out_opt_ LPCWSTR* PartName);

NTA_API
NTSTATUS NTAPI Path_ReleaseNtPath(
    _In_ PCUNICODE_STRING NtPath);

#pragma endregion

#pragma region File

typedef struct _FILE_FIND
{
    /* Initialize parameters */
    PCUNICODE_STRING SearchFilter;
    FILE_INFORMATION_CLASS FileInformationClass;
    HANDLE DirectoryHandle;

    /* Output data */
    BOOL HasData;
    DECLSPEC_ALIGN(4) PVOID Buffer;
    ULONG Length;
} FILE_FIND, *PFILE_FIND;

/// <summary>
/// Open a directory
/// </summary>
/// <seealso cref="NtOpenFile"/>
NTA_API
NTSTATUS NTAPI File_OpenDirectory(
    _Out_ PHANDLE DirectoryHandle,
    _In_ PCUNICODE_STRING DirectoryPath,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess);

NTA_API
NTSTATUS NTAPI File_Find(
    _In_ HANDLE DirectoryHandle,
    _Out_ PVOID Buffer,
    _In_ ULONG BufferSize,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _In_opt_ PCUNICODE_STRING SearchFilter,
    _In_ BOOLEAN RestartScan,
    _Out_ PBOOL HasData);

NTA_API
NTSTATUS NTAPI File_BeginFind(
    _Out_ PFILE_FIND FindData,
    _In_ HANDLE DirectoryHandle,
    _In_opt_ PCUNICODE_STRING SearchFilter,
    _In_ FILE_INFORMATION_CLASS FileInformationClass);

NTA_API
NTSTATUS NTAPI File_ContinueFind(
    _Inout_ PFILE_FIND FindData);

NTA_API
VOID NTAPI File_EndFind(
    _In_ PFILE_FIND FindData);

#pragma endregion

#pragma region Device

/// <summary>
/// Open a device
/// </summary>
/// <seealso cref="NtOpenFile"/>
NTA_API
NTSTATUS NTAPI Device_Open(
    _In_ PCUNICODE_STRING DeviceName,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE DeviceHandle);

/// <summary>
/// Query storage device property
/// </summary>
/// <remarks>Use <c>NTA_Free</c> to release the allocated buffer returned in <c>StorageProperty</c></remarks>
NTA_API
NTSTATUS NTAPI Device_QueryStorageProperty(
    _In_ HANDLE DeviceHandle,
    _In_ PSTORAGE_PROPERTY_QUERY Query,
    _In_ ULONG QuerySize,
    _Out_ _At_(*StoragePropertyLength,
               _Readable_bytes_(*StoragePropertyLength)
               _Writable_bytes_(*StoragePropertyLength)
               _Post_readable_byte_size_(*StoragePropertyLength)) PVOID* StorageProperty,
    _Out_opt_ PULONG StoragePropertyLength);

#pragma endregion

#pragma region Hardware

/// <summary>
/// Get firmware table
/// </summary>
/// <remarks>Use <c>NTA_Free</c> to release the allocated buffer returned in <c>FirmwareInformation</c></remarks>
NTA_API
NTSTATUS NTAPI HW_GetFirmwareTable(
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
