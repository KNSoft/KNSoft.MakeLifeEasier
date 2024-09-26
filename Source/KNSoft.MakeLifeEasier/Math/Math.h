#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

/// <summary>
/// Rounds a value
/// </summary>
#define Math_RoundUInt(val) ((UINT)((val) + 0.5))
#define Math_RoundInt(val) ((INT)((val) + ((val) > 0 ? 0.5 : -0.5)))
#define Math_RoundUIntPtr(val) ((UINT_PTR)((val) + 0.5))
#define Math_RoundIntPtr(val) ((INT_PTR)((val) + ((val) > 0 ? 0.5 : -0.5)))

/// <summary>
/// Check the number is a power of 2
/// </summary>
#define Math_IsPowerOf2(n) ((n != 0) && ((n & (n - 1)) == 0))

/// <summary>
/// Get the absolute difference between two numbers
/// </summary>
#define Math_AbsDiff(v1, v2) ((v1) > (v2) ? (v1) - (v2) : (v2) - (v1))

EXTERN_C_END
