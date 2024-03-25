#pragma once

#pragma comment(lib, "ntdll.lib")

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

/* Enable extensions */

#ifndef _NTASSASSIN_NDK_NO_EXTENSION
#include "Extension/Extension.h"
#endif

#ifndef _NTASSASSIN_NDK_NO_EXTENSION_CPU
#include "Extension/CPU.h"
#endif

#ifndef _NTASSASSIN_NDK_NO_EXTENSION_MSTOOLCHAIN
#include "Extension/MSToolChain.h"
#endif

#ifndef _NTASSASSIN_NDK_NO_EXTENSION_SMBIOS
#include "Extension/SMBIOS.h"
#endif
