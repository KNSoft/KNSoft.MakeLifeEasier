#include "NTAssassin.Lib.inl"

NTSTATUS NTAPI Path_ToNtPath(
    _In_z_ PCWSTR Path,
    _Out_ PUNICODE_STRING NtPath,
    _Out_opt_ LPCWSTR* PartName)
{
    return RtlDosPathNameToNtPathName_U_WithStatus(Path, NtPath, PartName, NULL);
}

NTSTATUS NTAPI Path_ReleaseNtPath(
    _In_ PCUNICODE_STRING NtPath)
{
    if (NtPath->Buffer != NULL)
    {
        RtlFreeHeap(NtGetProcessHeap(), 0, NtPath->Buffer);
    }
}
