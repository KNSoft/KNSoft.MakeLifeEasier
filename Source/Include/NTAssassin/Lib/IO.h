#pragma once

#include "../NT/MinDef.h"

#include <winioctl.h>

EXTERN_C_START

#pragma region File

NTSTATUS NTAPI File_OpenDirectory(
    _Out_ PHANDLE DirectoryHandle,
    _In_ PCUNICODE_STRING DirectoryPath,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess);

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

NTSTATUS NTAPI File_Find(
    _In_ HANDLE DirectoryHandle,
    _Out_ PVOID Buffer,
    _In_ ULONG BufferSize,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _In_opt_ PCUNICODE_STRING SearchFilter,
    _In_ BOOLEAN RestartScan,
    _Out_ PBOOL HasData);

NTSTATUS NTAPI File_BeginFind(
    _Out_ PFILE_FIND FindData,
    _In_ HANDLE DirectoryHandle,
    _In_opt_ PCUNICODE_STRING SearchFilter,
    _In_ FILE_INFORMATION_CLASS FileInformationClass);

NTSTATUS NTAPI File_ContinueFind(
    _Inout_ PFILE_FIND FindData);

VOID NTAPI File_EndFind(
    _In_ PFILE_FIND FindData);

#pragma endregion

#pragma region Device

NTSTATUS NTAPI Device_Open(
    _In_ PCUNICODE_STRING DeviceName,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE DeviceHandle);

NTSTATUS NTAPI Device_QueryStorageProperty(
    _In_ HANDLE DeviceHandle,
    _In_ PSTORAGE_PROPERTY_QUERY Query,
    _In_ ULONG QuerySize,
    _Out_ PVOID* AllocateBuffer);

VOID NTAPI Device_FreeStorageProperty(
    _Frees_ptr_ PVOID Buffer);

#pragma endregion

#pragma region Hardware

NTSTATUS NTAPI HW_GetFirmwareTable(
    _In_ ULONG ProviderSignature,
    _In_ ULONG TableID,
    _In_ SYSTEM_FIRMWARE_TABLE_ACTION Action,
    _Out_ PSYSTEM_FIRMWARE_TABLE_INFORMATION* FirmwareInformation,
    _Out_opt_ PULONG FirmwareInformationLength);

#pragma endregion

EXTERN_C_END
