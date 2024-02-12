#include "../Include/NTAssassin/NT/MinDef.h"

#if defined(_M_IX86)

DECLSPEC_NORETURN void __cdecl _except1()
{
    __fastfail(FAST_FAIL_FATAL_APP_EXIT);
}

#endif
