#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

_Success_(return != IMAGE_FILE_MACHINE_UNKNOWN)
FORCEINLINE
USHORT
PE_GetMachine(
    _In_ PVOID Image,
    _In_opt_ ULONG Size)
{
    LONG NtHeaderOffset;

    if (Size != 0 && Size < sizeof(IMAGE_DOS_HEADER))
    {
        goto _Fail;
    }
    NtHeaderOffset = ((PIMAGE_DOS_HEADER)Image)->e_lfanew;
    if (Size != 0 && Size <= NtHeaderOffset + sizeof(IMAGE_NT_HEADERS))
    {
        goto _Fail;
    }
    return ((PIMAGE_NT_HEADERS)Add2Ptr(Image, NtHeaderOffset))->FileHeader.Machine;

_Fail:
    return IMAGE_FILE_MACHINE_UNKNOWN;
}

_Success_(return != 0)
FORCEINLINE
USHORT
PE_GetMachineBits(
    _In_ USHORT Machine)
{
    if (Machine == IMAGE_FILE_MACHINE_AMD64 ||
        Machine == IMAGE_FILE_MACHINE_ARM64)
    {
        return 64;
    } else if (Machine == IMAGE_FILE_MACHINE_I386 ||
               Machine == IMAGE_FILE_MACHINE_ARM ||
               Machine == IMAGE_FILE_MACHINE_ARMNT)
    {
        return 32;
    }
    return 0;
}

/* Applies the load delta to a writable mapped PE32/PE32+ image.
   Supports the base relocation types handled by the Windows NT loader.
   A failure may leave earlier relocations applied. */
_Must_inspect_result_
FORCEINLINE
NTSTATUS
PE_RelocateImage(
    _Inout_ PVOID Image,
    _In_ LONG64 Delta)
{
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_DATA_DIRECTORY Directory;
    PIMAGE_BASE_RELOCATION Reloc;
    PUSHORT Entry;
    ULONG ImageSize, DirectoryCount, DirectorySize, Remaining, BlockSize, Count, i, Offset, Width, Step;
    USHORT Type, Machine;

    if (Delta == 0)
    {
        return STATUS_SUCCESS;
    }
    NtHeader = (PIMAGE_NT_HEADERS)Add2Ptr(Image, ((PIMAGE_DOS_HEADER)Image)->e_lfanew);
    if (NtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        PIMAGE_OPTIONAL_HEADER64 OptionalHeader = &((PIMAGE_NT_HEADERS64)NtHeader)->OptionalHeader;
        if (NtHeader->FileHeader.SizeOfOptionalHeader <
            RTL_SIZEOF_THROUGH_FIELD(IMAGE_OPTIONAL_HEADER64, NumberOfRvaAndSizes))
        {
            return STATUS_INVALID_IMAGE_FORMAT;
        }
        DirectoryCount = OptionalHeader->NumberOfRvaAndSizes;
        DirectorySize = RTL_SIZEOF_THROUGH_FIELD(IMAGE_OPTIONAL_HEADER64,
                                               DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC]);
        Directory = &OptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        ImageSize = OptionalHeader->SizeOfImage;
    } else if (NtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        PIMAGE_OPTIONAL_HEADER32 OptionalHeader = &((PIMAGE_NT_HEADERS32)NtHeader)->OptionalHeader;
        if (NtHeader->FileHeader.SizeOfOptionalHeader <
            RTL_SIZEOF_THROUGH_FIELD(IMAGE_OPTIONAL_HEADER32, NumberOfRvaAndSizes))
        {
            return STATUS_INVALID_IMAGE_FORMAT;
        }
        DirectoryCount = OptionalHeader->NumberOfRvaAndSizes;
        DirectorySize = RTL_SIZEOF_THROUGH_FIELD(IMAGE_OPTIONAL_HEADER32,
                                               DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC]);
        Directory = &OptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        ImageSize = OptionalHeader->SizeOfImage;
    } else
    {
        return STATUS_INVALID_IMAGE_FORMAT;
    }
    if (DirectoryCount <= IMAGE_DIRECTORY_ENTRY_BASERELOC ||
        NtHeader->FileHeader.SizeOfOptionalHeader < DirectorySize ||
        Directory->Size == 0 || Directory->VirtualAddress == 0 ||
        Directory->VirtualAddress > ImageSize || Directory->Size > ImageSize - Directory->VirtualAddress)
    {
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    Machine = NtHeader->FileHeader.Machine;
    Reloc = (PIMAGE_BASE_RELOCATION)Add2Ptr(Image, Directory->VirtualAddress);
    Remaining = Directory->Size;
    while (Remaining != 0)
    {
        if (Remaining < sizeof(*Reloc))
        {
            return STATUS_INVALID_IMAGE_FORMAT;
        }
        BlockSize = Reloc->SizeOfBlock;
        if (BlockSize < sizeof(*Reloc) || BlockSize > Remaining || BlockSize % sizeof(USHORT) != 0)
        {
            return STATUS_INVALID_IMAGE_FORMAT;
        }
        Count = (BlockSize - sizeof(*Reloc)) / sizeof(USHORT);
        Entry = (PUSHORT)(Reloc + 1);
        for (i = 0; i < Count; i += Step)
        {
            Type = Entry[i] >> 12;
            Offset = Entry[i] & 0xFFF;
            Step = 1;
            if (Type == IMAGE_REL_BASED_DIR64)
            {
                Width = sizeof(ULONG64);
            } else if (Type == IMAGE_REL_BASED_HIGHLOW)
            {
                Width = sizeof(ULONG);
            } else if (Type == IMAGE_REL_BASED_ABSOLUTE)
            {
                continue;
            } else if (Type == IMAGE_REL_BASED_HIGH || Type == IMAGE_REL_BASED_LOW)
            {
                Width = sizeof(USHORT);
            } else if (Type == IMAGE_REL_BASED_HIGHADJ)
            {
                if (i + 1 == Count)
                {
                    return STATUS_INVALID_IMAGE_FORMAT;
                }
                Width = sizeof(USHORT);
                /* The following WORD is data, not another relocation entry. */
                Step = 2;
            } else if (Type == IMAGE_REL_BASED_ARM_MOV32)
            {
                if (Machine != IMAGE_FILE_MACHINE_ARM &&
                    Machine != IMAGE_FILE_MACHINE_THUMB && Machine != IMAGE_FILE_MACHINE_ARMNT)
                {
                    return STATUS_INVALID_IMAGE_FORMAT;
                }
                Offset &= ~3UL;
                Width = 2 * sizeof(ULONG);
            } else if (Type == IMAGE_REL_BASED_THUMB_MOV32)
            {
                if (Machine != IMAGE_FILE_MACHINE_THUMB && Machine != IMAGE_FILE_MACHINE_ARMNT)
                {
                    return STATUS_INVALID_IMAGE_FORMAT;
                }
                Offset &= ~1UL;
                Width = 4 * sizeof(USHORT);
            } else
            {
                return STATUS_INVALID_IMAGE_FORMAT;
            }
            if (Reloc->VirtualAddress > ImageSize || Offset > ImageSize - Reloc->VirtualAddress ||
                Width > ImageSize - Reloc->VirtualAddress - Offset)
            {
                return STATUS_INVALID_IMAGE_FORMAT;
            }
            if (_Inline_LdrProcessRelocationBlockLongLong(Machine,
                                                         (ULONG_PTR)Image + Reloc->VirtualAddress,
                                                         Step,
                                                         Entry + i,
                                                         Delta) == NULL)
            {
                return STATUS_INVALID_IMAGE_FORMAT;
            }
        }
        Remaining -= BlockSize;
        Reloc = (PIMAGE_BASE_RELOCATION)((PBYTE)Reloc + BlockSize);
    }
    return STATUS_SUCCESS;
}

EXTERN_C_END
