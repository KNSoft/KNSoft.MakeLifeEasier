#include "../MakeLifeEasier.inl"

#pragma region Random

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

#pragma endregion
