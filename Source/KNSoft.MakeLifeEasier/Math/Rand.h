#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

/// <summary>
/// Generate hardware or software random
/// </summary>

/* See also Microsoft SymCrypt */

#if defined(_AMD64_) || defined(_X86_)

FORCEINLINE
LOGICAL
Math_GetHWRandom32(
    _Out_ PULONG Random)
{
    UINT i, p;

    for (i = 0; i < 1000000; i++)
    {
        if (_rdrand32_step(&p) != 0)
        {
            *Random = p;
            return TRUE;
        }
    }

    return FALSE;
}

FORCEINLINE
LOGICAL
Math_GetHWRandom64(
    _Out_ PULONGLONG Random)
{
    ULONG i;
    ULONGLONG p;

    for (i = 0; i < 1000000; i++)
    {
        if (
#if defined(_AMD64_)
            _rdrand64_step(&p) != 0
#else
            _rdrand32_step((PUINT)&p) != 0 && _rdrand32_step((PUINT)Add2Ptr(&p, sizeof(ULONG))) != 0
#endif
            )
        {
            *Random = p;
            return TRUE;
        }
    }

    return FALSE;
}

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

FORCEINLINE
ULONG
Math_Random32(VOID)
{
#if defined(_AMD64_) || defined(_X86_)
    ULONG p;
    return Math_GetHWRandom32(&p) ? p : Math_GetSWRandom32();
#else
    return Math_GetSWRandom32();
#endif
}

FORCEINLINE
ULONGLONG
Math_Random64(VOID)
{
#if defined(_AMD64_) || defined(_X86_)
    ULONGLONG p;
    return Math_GetHWRandom64(&p) ? p : Math_GetSWRandom64();
#else
    return Math_GetSWRandom64();
#endif
}

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
    return Min + Math_Random32() % (Max - Min + 1);
}

FORCEINLINE
ULONGLONG
Math_RangedRandom64(
    _In_ ULONGLONG Min,
    _In_range_(Min, MAXULONGLONG) ULONGLONG Max)
{
    return Min + Math_Random64() % (Max - Min + 1);
}

EXTERN_C_END
