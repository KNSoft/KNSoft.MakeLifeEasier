#include "NTAssassin.Lib.inl"

NTSTATUS NTAPI Device_Open(_In_ PCUNICODE_STRING DeviceName, _In_ ACCESS_MASK DesiredAccess, _Out_ PHANDLE DeviceHandle)
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
