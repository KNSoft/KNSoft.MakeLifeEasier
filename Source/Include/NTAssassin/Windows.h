#pragma once

#ifndef _NTASSASSIN_WINDOWS_
#define _NTASSASSIN_WINDOWS_

#include "NT/NT.h"

/* Windows.h */

#include <Windows.h>
#include "WinDef/Addendum/WinUser.h"
#include "WinDef/Addendum/winsta.h"

/* APIs */

#include "WinDef/API/Ntdll.h"
#include "WinDef/API/Kernel32.h"
#include "WinDef/API/User32.h"
#include "WinDef/API/WinSta.h"

/* Additional headers */

#include <intrin.h>
#include <suppress.h>

/* Always ignore Microsoft extension warnings */

// nonstandard extension used: zero-sized array in struct/union
#pragma warning(disable: 4200)

/* Enable extensions */

#ifndef _NTASSASSIN_NO_EXTENSION_

#include "Extension/Extension.h"

#endif

#endif /* _NTASSASSIN_WINDOWS_ */
