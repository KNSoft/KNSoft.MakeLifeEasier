#pragma once

#include "../MakeLifeEasier.h"

#include <KNSoft/NDK/Package/RandGen.inl>

EXTERN_C_START

/// <summary>
/// Generates a random within specified range
/// </summary>
/// <param name="Min">Minimum</param>
/// <param name="Max">Maximum</param>
/// <returns>A random number within [Min, Max]</returns>

FORCEINLINE
ULONG
Math_RangedRandom32(
    _In_ ULONG Min,
    _In_range_(Min, MAXULONG) ULONG Max)
{
    return Min + Rand_32() % (Max - Min + 1);
}

FORCEINLINE
ULONGLONG
Math_RangedRandom64(
    _In_ ULONGLONG Min,
    _In_range_(Min, MAXULONGLONG) ULONGLONG Max)
{
    return Min + Rand_64() % (Max - Min + 1);
}

EXTERN_C_END
