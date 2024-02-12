#include "../Test.h"

#include <winioctl.h>

static PCSTR g_pszDescription = "This sample queries the first hard disk volume information.";

_Must_inspect_result_
_Ret_maybenull_
static PVOID IO_QueryStorageProperty(
    _In_ HANDLE DeviceHandle,
    _In_ PSTORAGE_PROPERTY_QUERY Query,
    _In_ ULONG QuerySize)
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
        goto _fail_0;
    }

    /* Allocate buffer to receive data */
    Buffer = RtlAllocateHeap(CURRENT_PROCESS_HEAP, 0, sdh.Size);
    if (Buffer == NULL)
    {
        return NULL;
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
    if (!NT_SUCCESS(Status))
    {
        goto _fail_1;
    }

    return Buffer;

_fail_1:
    RtlFreeHeap(CURRENT_PROCESS_HEAP, 0, Buffer);
_fail_0:
    NtSetLastStatus(Status);
    return NULL;
}

static VOID IO_FreeStorageProperty(_Frees_ptr_ PVOID Buffer)
{
    RtlFreeHeap(CURRENT_PROCESS_HEAP, 0, Buffer);
}

static UNICODE_STRING g_usQueryVolumeDeviceName = RTL_CONSTANT_STRING(L"\\Device\\HarddiskVolume1");

BOOL Sample_QueryStorageProperty()
{
    HANDLE DeviceHandle;
    PSTORAGE_DEVICE_DESCRIPTOR psdd;
    STORAGE_PROPERTY_QUERY spq = {
        .PropertyId = StorageDeviceProperty,
        .QueryType = PropertyStandardQuery
    };

    PrintTitle(__FUNCTION__, g_pszDescription);

    /* Open Device */
    DeviceHandle = IO_OpenDevice(&g_usQueryVolumeDeviceName, FILE_READ_ATTRIBUTES | SYNCHRONIZE);
    if (!DeviceHandle)
    {
        PrintF("IO_OpenDevice failed with: 0x%08lX\n", NtGetLastStatus());
        return FALSE;
    }

    /* Query Property */
    psdd = IO_QueryStorageProperty(DeviceHandle, &spq, sizeof(spq));
    if (!psdd)
    {
        PrintF("IO_QueryStorageProperty failed with: 0x%08lX\n", NtGetLastStatus());
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

    IO_FreeStorageProperty(psdd);
    NtClose(DeviceHandle);
    return TRUE;
}
