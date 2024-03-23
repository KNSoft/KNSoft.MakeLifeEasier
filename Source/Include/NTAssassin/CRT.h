#pragma once

#define _NO_CRT_STDIO_INLINE

#include "NDK/NT/MinDef.h"

#if defined(__cplusplus) && defined(_MT)
#if !defined(_DEBUG) && !defined(_DLL)
#pragma comment(linker, "/NODEFAULTLIB:libcpmt.lib")
#elif !defined(_DEBUG) && defined(_DLL)
#pragma comment(linker, "/NODEFAULTLIB:msvcprt.lib")
#elif defined(_DEBUG) && !defined(_DLL)
#pragma comment(linker, "/NODEFAULTLIB:libcpmtd.lib")
#elif defined(_DEBUG) && defined(_DLL)
#pragma comment(linker, "/NODEFAULTLIB:msvcprtd.lib")
#endif
#endif

EXTERN_C NTSTATUS NTAPI ExeMain();

EXTERN_C INT __argc;
EXTERN_C CHAR** __argv;
EXTERN_C WCHAR** __wargv;
EXTERN_C CHAR* _acmdln;
EXTERN_C WCHAR* _wcmdln;
