#include "../MakeLifeEasier.inl"

#pragma region Random

/* See also Microsoft SymCrypt */

#if defined(_AMD64_) || defined(_X86_)

LOGICAL
NTAPI
Math_GetHWRandom32(
    _Out_ PULONG Random)
{
    ULONG i, p;

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

LOGICAL
NTAPI
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
            _rdrand32_step((PULONG)&p) != 0 && _rdrand32_step((PULONG)Add2Ptr(&p, sizeof(ULONG))) != 0
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

static ULONG g_ulRandSeed = 0;

ULONG
NTAPI
Math_GetSWRandom32(VOID)
{
    return ((RtlRandomEx(&g_ulRandSeed) & 0xFFFF) << 16) | (RtlRandomEx(&g_ulRandSeed) & 0xFFFF);
}

ULONGLONG
NTAPI
Math_GetSWRandom64(VOID)
{
    ULONGLONG p;

    p = (RtlRandomEx(&g_ulRandSeed) & 0xFFFFFFULL) << 40;
    p |= (RtlRandomEx(&g_ulRandSeed) & 0xFFFFFFULL) << 16;
    p |= RtlRandomEx(&g_ulRandSeed) & 0xFFFFULL;
    return p;
}

ULONG
NTAPI
Math_Random32(VOID)
{
    ULONG p;
    return
#if defined(_AMD64_) || defined(_X86_)
        Math_GetHWRandom32(&p) ? p :
#endif
        Math_GetSWRandom32();
}

ULONGLONG
NTAPI
Math_Random64(VOID)
{
    ULONGLONG p;
    return
#if defined(_AMD64_) || defined(_X86_)
        Math_GetHWRandom64(&p) ? p :
#endif
        Math_GetSWRandom64();
}

ULONG
NTAPI
Math_RangedRandom32(
    _In_ ULONG Min,
    _In_range_(Min, MAXULONG) ULONG Max)
{
    return Min + Math_Random32() % (Max - Min + 1);
}

ULONGLONG
NTAPI
Math_RangedRandom64(
    _In_ ULONGLONG Min,
    _In_range_(Min, MAXULONGLONG) ULONGLONG Max)
{
    return Min + Math_Random64() % (Max - Min + 1);
}

#pragma endregion
