#include "GS.inl"

#define CPU_CACHE_LINE_SIZE 64
#define _STATIC_ASSERT(expr) static_assert((expr), #expr)

union __declspec(align(CPU_CACHE_LINE_SIZE))
{
    uintptr_t value;
} __security_cookie = { DEFAULT_SECURITY_COOKIE };
_STATIC_ASSERT(sizeof(__security_cookie) == CPU_CACHE_LINE_SIZE);

uintptr_t __security_cookie_complement = ~(DEFAULT_SECURITY_COOKIE);
