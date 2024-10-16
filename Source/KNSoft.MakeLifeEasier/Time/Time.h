#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

MLE_API
VOID
NTAPI
Time_UnixTimeToFileTime(
    _In_ time_t UnixTime,
    _Out_ PFILETIME FileTime);

/// <summary>
/// Stopwatch for elapsed time measurement
/// </summary>
/// <param name="PrevTime">Previous time</param>
/// <param name="Multiplier">
/// Multiplier to convert second to other units,
/// 1e3 for millisecond, 1e6 for microsecond, ...
/// </param>

FORCEINLINE
ULONGLONG
Time_StopWatchStart(VOID)
{
    LARGE_INTEGER liCounter;

    NtQueryPerformanceCounter(&liCounter, NULL);
    return (ULONGLONG)liCounter.QuadPart;
}

FORCEINLINE
ULONGLONG
Time_StopWatchStop(
    _In_ ULONGLONG PrevTime,
    _In_ ULONG Multiplier)
{
    LARGE_INTEGER liCounter, liFreq;

    NtQueryPerformanceCounter(&liCounter, &liFreq);
    return (ULONGLONG)((((ULONGLONG)liCounter.QuadPart - PrevTime) * Multiplier / (DOUBLE)liFreq.QuadPart) + (DOUBLE)0.5);
}

EXTERN_C_END
