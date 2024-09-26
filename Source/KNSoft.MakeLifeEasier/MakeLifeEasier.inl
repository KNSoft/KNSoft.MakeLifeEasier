#pragma once

#define OEMRESOURCE
#define STRICT_TYPED_ITEMIDS

#include <KNSoft/NDK/NDK.h>

#ifdef _WINDLL
#define MLE_API DECLSPEC_EXPORT
#else
#define MLE_API
#endif

#include "MakeLifeEasier.h"
