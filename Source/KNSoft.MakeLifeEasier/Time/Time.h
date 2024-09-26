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

MLE_API
ULONGLONG
NTAPI
Time_StopWatchStart(VOID);

MLE_API
ULONGLONG
NTAPI
Time_StopWatchStop(
    _In_ ULONGLONG PrevTime,
    _In_ ULONG Multiplier);

EXTERN_C_END
