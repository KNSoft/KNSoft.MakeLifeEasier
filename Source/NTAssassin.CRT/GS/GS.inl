#pragma once

#ifndef _UINTPTR_T_DEFINED
#define _UINTPTR_T_DEFINED
#ifdef _WIN64
typedef unsigned __int64  uintptr_t;
#else
typedef unsigned int uintptr_t;
#endif
#endif

#ifdef _WIN64
#define DEFAULT_SECURITY_COOKIE ((uintptr_t)0x00002B992DDFA232)
#else
#define DEFAULT_SECURITY_COOKIE ((uintptr_t)0xBB40E64E)
#endif
