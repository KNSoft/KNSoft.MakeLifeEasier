#pragma once

#include "MinDef.h"

#pragma region Pointer and Size

#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))
#define PtrOffset(B,O) ((ULONG)((ULONG_PTR)(O) - (ULONG_PTR)(B)))

#define ROUND_TO_SIZE(_length, _alignment) \
            ((((ULONG_PTR)(_length)) + ((_alignment)-1)) & ~(ULONG_PTR) ((_alignment) - 1))

#define IS_ALIGNED(_pointer, _alignment) \
        ((((ULONG_PTR) (_pointer)) & ((_alignment) - 1)) == 0)

#pragma endregion

#pragma region Flag

#ifndef FlagOn
#define FlagOn(_F,_SF) ((_F) & (_SF))
#endif

#ifndef BooleanFlagOn
#define BooleanFlagOn(F,SF) ((BOOLEAN)(((F) & (SF)) != 0))
#endif

#ifndef SetFlag
#define SetFlag(_F,_SF) ((_F) |= (_SF))
#endif

#ifndef ClearFlag
#define ClearFlag(_F,_SF) ((_F) &= ~(_SF))
#endif

#pragma endregion

#pragma region Pseudo Handles

#define NtCurrentProcess() ((HANDLE)(LONG_PTR)-1)  
#define ZwCurrentProcess() NtCurrentProcess()
#define NtCurrentThread() ((HANDLE)(LONG_PTR)-2)   
#define ZwCurrentThread() NtCurrentThread()
#define NtCurrentSession() ((HANDLE)(LONG_PTR)-3)  
#define ZwCurrentSession() NtCurrentSession()

#if defined(_NTASSASSIN_NTDEF_EXTENSION_)

#define NtCurrentProcessToken()  ((HANDLE)(LONG_PTR)-4)
#define ZwCurrentProcessToken() NtCurrentProcessToken()
#define NtCurrentThreadToken() ((HANDLE)(LONG_PTR)-5)
#define ZwCurrentThreadToken() NtCurrentThreadToken()
#define NtCurrentThreadEffectiveToken() ((HANDLE)(LONG_PTR)-6)
#define ZwCurrentThreadEffectiveToken() NtCurrentThreadEffectiveToken()

#endif /* defined(_NTASSASSIN_NTDEF_EXTENSION_) */

#define SERVERNAME_CURRENT ((HANDLE)NULL)

#pragma endregion
