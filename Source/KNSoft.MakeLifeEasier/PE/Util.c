#include "../MakeLifeEasier.inl"

_Must_inspect_result_
NTSTATUS
PE_RelocateImage(
    _Inout_ PVOID Image,
    _In_ LONG64 Delta)
{
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_DATA_DIRECTORY Directory;
    PIMAGE_BASE_RELOCATION Reloc;
    ULONG Remaining, BlockSize, Count;
    USHORT Machine;

    if (Delta == 0)
    {
        return STATUS_SUCCESS;
    }
    NtHeader = (PIMAGE_NT_HEADERS)Add2Ptr(Image, ((PIMAGE_DOS_HEADER)Image)->e_lfanew);
    if (NtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        PIMAGE_OPTIONAL_HEADER64 OptionalHeader = &((PIMAGE_NT_HEADERS64)NtHeader)->OptionalHeader;
        Directory = OptionalHeader->NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_BASERELOC ?
            &OptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC] : NULL;
    } else if (NtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        PIMAGE_OPTIONAL_HEADER32 OptionalHeader = &((PIMAGE_NT_HEADERS32)NtHeader)->OptionalHeader;
        Directory = OptionalHeader->NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_BASERELOC ?
            &OptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC] : NULL;
    } else
    {
        return STATUS_INVALID_IMAGE_FORMAT;
    }
    if (Directory == NULL || Directory->VirtualAddress == 0 || Directory->Size == 0)
    {
        return STATUS_SUCCESS;
    }
    Machine = NtHeader->FileHeader.Machine;
    Reloc = (PIMAGE_BASE_RELOCATION)Add2Ptr(Image, Directory->VirtualAddress);
    Remaining = Directory->Size;
    while (Remaining != 0)
    {
        BlockSize = Reloc->SizeOfBlock;
        Count = (BlockSize - sizeof(*Reloc)) / sizeof(USHORT);
        if (_Inline_LdrProcessRelocationBlockLongLong(Machine,
                                                      (ULONG_PTR)Image + Reloc->VirtualAddress,
                                                      Count,
                                                      (PUSHORT)(Reloc + 1),
                                                      Delta) == NULL)
        {
            return STATUS_INVALID_IMAGE_FORMAT;
        }
        Remaining -= BlockSize;
        Reloc = (PIMAGE_BASE_RELOCATION)Add2Ptr(Reloc, BlockSize);
    }
    return STATUS_SUCCESS;
}
