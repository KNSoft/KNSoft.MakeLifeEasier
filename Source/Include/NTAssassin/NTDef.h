#pragma once

#ifndef _NTASSASSIN_NTDEF_
#define _NTASSASSIN_NTDEF_

#include "NT/MinDef.h"

/* Windows.h */

#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME

#define OEMRESOURCE

#include <Windows.h>
#include "Addendum/WinUser.h"
#include "Addendum/winsta.h"

/* Additional headers */

#include <intrin.h>
#include <suppress.h>

/* Always ignore Microsoft extension warnings */

// nonstandard extension used: zero-sized array in struct/union
#pragma warning(disable: 4200)

/* NT support */

#include "NT/Types.h"
#include "NT/Macro.h"

/* APIs */

#include "API/Ntdll.h"
#include "API/Kernel32.h"
#include "API/User32.h"
#include "API/WinSta.h"

#endif /* _NTASSASSIN_NTDEF_ */
