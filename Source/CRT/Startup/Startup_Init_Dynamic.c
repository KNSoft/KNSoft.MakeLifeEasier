#include "Startup.inl"

_CRTALLOC(".CRT$XIA") _PIFV __xi_a[] = { NULL }; // C initializers (first)
_CRTALLOC(".CRT$XIZ") _PIFV __xi_z[] = { NULL }; // C initializers (last)
_CRTALLOC(".CRT$XCA") _PVFV __xc_a[] = { NULL }; // C++ initializers (first)
_CRTALLOC(".CRT$XCZ") _PVFV __xc_z[] = { NULL }; // C++ initializers (last)
_CRTALLOC(".CRT$XPA") _PVFV __xp_a[] = { NULL }; // C pre-terminators (first)
_CRTALLOC(".CRT$XPZ") _PVFV __xp_z[] = { NULL }; // C pre-terminators (last)
_CRTALLOC(".CRT$XTA") _PVFV __xt_a[] = { NULL }; // C terminators (first)
_CRTALLOC(".CRT$XTZ") _PVFV __xt_z[] = { NULL }; // C terminators (last)

#pragma comment(linker, "/merge:.CRT=.rdata")

void __cdecl _initterm(_PVFV* const first, _PVFV* const last)
{
    _PVFV* Func;

    for (Func = first; Func != last; Func++)
    {
        if (*Func != NULL)
        {
            (**Func)();
        }
    }
}

int __cdecl _initterm_e(_PIFV* const first, _PIFV* const last)
{
    _PIFV* Func;
    int iRet;

    for (Func = first; Func != last; Func++)
    {
        if (*Func != NULL)
        {
            iRet = (**Func)();
            if (iRet != 0)
            {
                return iRet;
            }
        }
    }

    return 0;
}
