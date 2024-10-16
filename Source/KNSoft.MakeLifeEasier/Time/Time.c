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
