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

EXTERN_C_END
