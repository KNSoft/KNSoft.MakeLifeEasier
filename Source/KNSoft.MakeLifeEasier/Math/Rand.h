#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

/// <summary>
/// Generate hardware or software random
/// </summary>

#if defined(_AMD64_) || defined(_X86_)

MLE_API
LOGICAL
NTAPI
Math_GetHWRandom32(
    _Out_ PULONG Random);

MLE_API
LOGICAL
NTAPI
Math_GetHWRandom64(
    _Out_ PULONGLONG Random);

#endif

MLE_API
ULONG
NTAPI
Math_GetSWRandom32(VOID);

MLE_API
ULONGLONG
NTAPI
Math_GetSWRandom64(VOID);

/// <summary>
/// Generates a random
/// </summary>
/// <returns>All bits of return value are randomized and always success</returns>

MLE_API
ULONG
NTAPI
Math_Random32(VOID);

MLE_API
ULONGLONG
NTAPI
Math_Random64(VOID);

/// <summary>
/// Generates a random within specified range
/// </summary>
/// <param name="Min">Minimum</param>
/// <param name="Max">Maximum</param>
/// <returns>A random number within [Min, Max]</returns>

MLE_API
ULONG
NTAPI
Math_RangedRandom32(
    _In_ ULONG Min,
    _In_range_(Min, MAXULONG) ULONG Max);

MLE_API
ULONGLONG
NTAPI
Math_RangedRandom64(
    _In_ ULONGLONG Min,
    _In_range_(Min, MAXULONGLONG) ULONGLONG Max);

EXTERN_C_END
