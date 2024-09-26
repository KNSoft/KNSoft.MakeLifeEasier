#include "../MakeLifeEasier.inl"

VOID
NTAPI
Time_UnixTimeToFileTime(
    _In_ time_t UnixTime,
    _Out_ PFILETIME FileTime)
{
    ULARGE_INTEGER Time;

    Time.QuadPart = (UnixTime * 10000000LL) + 116444736000000000LL;
    FileTime->dwLowDateTime = Time.LowPart;
    FileTime->dwHighDateTime = Time.HighPart;
}

ULONGLONG
NTAPI
Time_StopWatchStart(VOID)
{
    LARGE_INTEGER liCounter;

    NtQueryPerformanceCounter(&liCounter, NULL);
    return (ULONGLONG)liCounter.QuadPart;
}

ULONGLONG
NTAPI
Time_StopWatchStop(
    _In_ ULONGLONG PrevTime,
    _In_ ULONG Multiplier)
{
    LARGE_INTEGER liCounter, liFreq;

    NtQueryPerformanceCounter(&liCounter, &liFreq);
    return (ULONGLONG)((((ULONGLONG)liCounter.QuadPart - PrevTime) * Multiplier / (DOUBLE)liFreq.QuadPart) + (DOUBLE)0.5);
}
