#pragma once

/* Include NDK */

#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME

#define OEMRESOURCE
#define STRICT_TYPED_ITEMIDS

#include "NDK/NDK.h"

/* Include CRT if needed */

#if defined(_VC_NODEFAULTLIB) && !defined(_NTASSASSIN_NO_CRT)
#include "CRT.h"
#endif

/* Include Lib */

#include "Lib/Lib.h"
