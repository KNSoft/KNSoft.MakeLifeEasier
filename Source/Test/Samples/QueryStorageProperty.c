#include "../Test.h"

static PCSTR g_pszDescription = "This sample queries the first hard disk volume information.";

static CONST UNICODE_STRING g_usQueryVolumeDeviceName = RTL_CONSTANT_STRING(L"\\Device\\HarddiskVolume1");

BOOL Sample_QueryStorageProperty()
{
    NTSTATUS Status;
    HANDLE DeviceHandle;
    PSTORAGE_DEVICE_DESCRIPTOR psdd;
    STORAGE_PROPERTY_QUERY spq = {
        .PropertyId = StorageDeviceProperty,
        .QueryType = PropertyStandardQuery
    };

    PrintTitle(__FUNCTION__, g_pszDescription);

    /* Open Device */
    Status = Device_Open(&g_usQueryVolumeDeviceName, FILE_READ_ATTRIBUTES | SYNCHRONIZE, &DeviceHandle);
    if (!NT_SUCCESS(Status))
    {
        PrintF("Device_Open failed with: 0x%08lX\n", Status);
        return FALSE;
    }

    /* Query Property */
    Status = Device_QueryStorageProperty(DeviceHandle, &spq, sizeof(spq), &psdd);
    if (!NT_SUCCESS(Status))
    {
        PrintF("Device_QueryStorageProperty failed with: 0x%08lX\n", Status);
        NtClose(DeviceHandle);
        return FALSE;
    }

    /* Print Strings */
    PrintF("Properties of Storage device: %wZ\n", &g_usQueryVolumeDeviceName);
    if (psdd->VendorIdOffset != 0)
    {
        PrintF("\tVendorId: %hs\n", (PCSTR)Add2Ptr(psdd, psdd->VendorIdOffset));
    }
    if (psdd->ProductIdOffset != 0)
    {
        PrintF("\tProductId: %hs\n", (PCSTR)Add2Ptr(psdd, psdd->ProductIdOffset));
    }
    if (psdd->ProductRevisionOffset != 0)
    {
        PrintF("\tProductRevision: %hs\n", (PCSTR)Add2Ptr(psdd, psdd->ProductRevisionOffset));
    }
    if (psdd->SerialNumberOffset != 0)
    {
        PrintF("\tSerialNumber: %hs\n", (PCSTR)Add2Ptr(psdd, psdd->SerialNumberOffset));
    }

    Device_FreeStorageProperty(psdd);
    NtClose(DeviceHandle);
    return TRUE;
}
