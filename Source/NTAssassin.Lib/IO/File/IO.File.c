#include "NTAssassin.Lib.inl"

NTSTATUS NTAPI File_OpenDirectory(
    _Out_ PHANDLE DirectoryHandle,
    _In_ PCUNICODE_STRING DirectoryPath,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess)
{
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(DirectoryPath, OBJ_CASE_INSENSITIVE);

    return NtOpenFile(DirectoryHandle,
                      DesiredAccess,
                      &ObjectAttributes,
                      &IoStatusBlock,
                      ShareAccess,
                      FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
}
