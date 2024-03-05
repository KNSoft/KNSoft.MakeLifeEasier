#include "GS.inl"
#include "../CRT.inl"

#include <vcruntime_internal.h>

#if defined(_M_X64)
#define GET_TICK_COUNT_FAST() (__umulh((ULONGLONG)SharedUserData->TickCountMultiplier << 32, SharedUserData->TickCountQuad << 8))
#else
#define GET_TICK_COUNT_FAST()\
    (((SharedUserData->TickCountMultiplier * (ULONGLONG)SharedUserData->TickCount.High1Time) << 8) +\
    ((SharedUserData->TickCountMultiplier * (ULONGLONG)SharedUserData->TickCount.LowPart) >> 24))
#endif

extern uintptr_t __security_cookie_complement;

void __cdecl __security_init_cookie()
{
    UINT_PTR Cookie;
    LARGE_INTEGER PerfCounter, PerfFreq;

    /* Skip if cookie is already initialized and good */
    if (__security_cookie != DEFAULT_SECURITY_COOKIE
#if defined (_M_IX86)
        && (__security_cookie & 0xFFFF0000) != 0
#endif
       )
    {
        __security_cookie_complement = ~__security_cookie;
        return;
    }

    /* Increase entropy using system time */
#if defined (_WIN64)
    Cookie = *(UINT_PTR*)&(SharedUserData->SystemTime);
#else
    Cookie = SharedUserData->SystemTime.LowPart;
    Cookie ^= SharedUserData->SystemTime.High1Time;
#endif

    /* Increase entropy using PID and TID */
    Cookie ^= MAKEQWORD(NtGetCurrentThreadId(), NtGetCurrentProcessId());

    /* Increase entropy using tick count */
#if defined (_WIN64)
    Cookie ^= (((UINT_PTR)GET_TICK_COUNT_FAST()) << 56);
#endif
    Cookie ^= (UINT_PTR)GET_TICK_COUNT_FAST();

    /* Increase entropy using performance counter */
    NtQueryPerformanceCounter(&PerfCounter, &PerfFreq);
#if defined (_WIN64)
    Cookie ^= ((UINT_PTR)PerfCounter.LowPart << 32) ^ PerfCounter.QuadPart;
#else
    Cookie ^= PerfCounter.LowPart;
    Cookie ^= PerfCounter.HighPart;
#endif

    /* Increase entropy using stack ASLR */
    Cookie ^= (UINT_PTR)&Cookie;

#if defined (_WIN64)
    Cookie &= 0x0000FFFFFFFFFFFF;
#endif

    /* Adjust cookie if it seems not good enough */
    if (Cookie == DEFAULT_SECURITY_COOKIE)
    {
        Cookie = DEFAULT_SECURITY_COOKIE + 1;
    }
#if defined (_M_IX86)
    else if ((Cookie & 0xFFFF0000) == 0)
    {
        Cookie |= ((Cookie | 0x4711) << 16);
    }
#endif

    /* Done */
    __security_cookie = Cookie;
    __security_cookie_complement = ~Cookie;
}

#pragma optimize("", off)

#if defined _M_IX86
#define GSFAILURE_PARAMETER
#else
#define GSFAILURE_PARAMETER _In_ uintptr_t stack_cookie
#endif

__declspec(noreturn) void __cdecl __report_gsfailure(GSFAILURE_PARAMETER)
{
    __fastfail(FAST_FAIL_STACK_COOKIE_CHECK_FAILURE);
}

DECLSPEC_NORETURN void __cdecl __report_securityfailure(unsigned long Code)
{
    __fastfail(Code);
}

DECLSPEC_NORETURN void __cdecl __report_rangecheckfailure()
{
    __report_securityfailure(FAST_FAIL_RANGE_CHECK_FAILURE);
}
