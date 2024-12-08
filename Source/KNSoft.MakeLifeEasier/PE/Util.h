#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

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
