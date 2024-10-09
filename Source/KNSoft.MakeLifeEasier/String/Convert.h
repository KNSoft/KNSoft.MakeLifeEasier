#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

#pragma region String To Integer

MLE_API
_Success_(return != FALSE)
LOGICAL
NTAPI
Str_ToIntExW(
    _In_ PCWSTR StrValue,
    _In_ BOOL Unsigned,
    _In_ ULONG Base,
    _Out_writes_bytes_(ValueSize) PVOID Value,
    _In_ ULONG ValueSize);

MLE_API
_Success_(return != FALSE)
LOGICAL
NTAPI
Str_ToIntExA(
    _In_ PCSTR StrValue,
    _In_ BOOL Unsigned,
    _In_ ULONG Base,
    _Out_writes_bytes_(ValueSize) PVOID Value,
    _In_ ULONG ValueSize);

#define Str_ToIntW(StrValue, Value) Str_ToIntExW(StrValue, FALSE, 0, Value, sizeof(*(Value)))
#define Str_ToIntA(StrValue, Value) Str_ToIntExA(StrValue, FALSE, 0, Value, sizeof(*(Value)))
#define Str_ToUIntW(StrValue, Value) Str_ToIntExW(StrValue, TRUE, 0, Value, sizeof(*(Value)))
#define Str_ToUIntA(StrValue, Value) Str_ToIntExA(StrValue, TRUE, 0, Value, sizeof(*(Value)))
#define Str_HexToIntW(StrValue, Value) Str_ToIntExW(StrValue, FALSE, 16, Value, sizeof(*(Value)))
#define Str_HexToIntA(StrValue, Value) Str_ToIntExA(StrValue, FALSE, 16, Value, sizeof(*(Value)))
#define Str_HexToUIntW(StrValue, Value) Str_ToIntExW(StrValue, TRUE, 16, Value, sizeof(*(Value)))
#define Str_HexToUIntA(StrValue, Value) Str_ToIntExA(StrValue, TRUE, 16, Value, sizeof(*(Value)))
#define Str_DecToIntW(StrValue, Value) Str_ToIntExW(StrValue, FALSE, 10, Value, sizeof(*(Value)))
#define Str_DecToIntA(StrValue, Value) Str_ToIntExA(StrValue, FALSE, 10, Value, sizeof(*(Value)))
#define Str_DecToUIntW(StrValue, Value) Str_ToIntExW(StrValue, TRUE, 10, Value, sizeof(*(Value)))
#define Str_DecToUIntA(StrValue, Value) Str_ToIntExA(StrValue, TRUE, 10, Value, sizeof(*(Value)))
#define Str_OctToIntW(StrValue, Value) Str_ToIntExW(StrValue, FALSE, 8, Value, sizeof(*(Value)))
#define Str_OctToIntA(StrValue, Value) Str_ToIntExA(StrValue, FALSE, 8, Value, sizeof(*(Value)))
#define Str_OctToUIntW(StrValue, Value) Str_ToIntExW(StrValue, TRUE, 8, Value, sizeof(*(Value)))
#define Str_OctToUIntA(StrValue, Value) Str_ToIntExA(StrValue, TRUE, 8, Value, sizeof(*(Value)))
#define Str_BinToIntW(StrValue, Value) Str_ToIntExW(StrValue, FALSE, 2, Value, sizeof(*(Value)))
#define Str_BinToIntA(StrValue, Value) Str_ToIntExA(StrValue, FALSE, 2, Value, sizeof(*(Value)))
#define Str_BinToUIntW(StrValue, Value) Str_ToIntExW(StrValue, TRUE, 2, Value, sizeof(*(Value)))
#define Str_BinToUIntA(StrValue, Value) Str_ToIntExA(StrValue, TRUE, 2, Value, sizeof(*(Value)))

#pragma endregion

#pragma region String From Integer

MLE_API
_Success_(return != FALSE)
LOGICAL
NTAPI
Str_FromIntExW(
    _In_ INT64 Value,
    _In_ BOOL Unsigned,
    _In_ UINT Base,
    _Out_writes_(DestCchSize) PWSTR StrValue,
    _In_ ULONG DestCchSize);

MLE_API
_Success_(return != FALSE)
LOGICAL
NTAPI
Str_FromIntExA(
    _In_ INT64 Value,
    _In_ BOOL Unsigned,
    _In_ UINT Base,
    _Out_writes_(DestCchSize) PSTR StrValue,
    _In_ ULONG DestCchSize);

#define Str_FromIntW(Value, StrValue) Str_FromIntExW(Value, FALSE, 0, StrValue, ARRAYSIZE(StrValue))
#define Str_FromIntA(Value, StrValue) Str_FromIntExA(Value, FALSE, 0, StrValue, ARRAYSIZE(StrValue))
#define Str_FromUIntW(Value, StrValue) Str_FromIntExW(Value, TRUE, 0, StrValue, ARRAYSIZE(StrValue))
#define Str_FromUIntA(Value, StrValue) Str_FromIntExA(Value, TRUE, 0, StrValue, ARRAYSIZE(StrValue))
#define Str_HexFromIntW(Value, StrValue) Str_FromIntExW(Value, FALSE, 16, StrValue, ARRAYSIZE(StrValue))
#define Str_HexFromIntA(Value, StrValue) Str_FromIntExA(Value, FALSE, 16, StrValue, ARRAYSIZE(StrValue))
#define Str_HexFromUIntW(Value, StrValue) Str_FromIntExW(Value, TRUE, 16, StrValue, ARRAYSIZE(StrValue))
#define Str_HexFromUIntA(Value, StrValue) Str_FromIntExA(Value, TRUE, 16, StrValue, ARRAYSIZE(StrValue))
#define Str_DecFromIntW(Value, StrValue) Str_FromIntExW(Value, FALSE, 10, StrValue, ARRAYSIZE(StrValue))
#define Str_DecFromIntA(Value, StrValue) Str_FromIntExA(Value, FALSE, 10, StrValue, ARRAYSIZE(StrValue))
#define Str_DecFromUIntW(Value, StrValue) Str_FromIntExW(Value, TRUE, 10, StrValue, ARRAYSIZE(StrValue))
#define Str_DecFromUIntA(Value, StrValue) Str_FromIntExA(Value, TRUE, 10, StrValue, ARRAYSIZE(StrValue))
#define Str_OctFromIntW(Value, StrValue) Str_FromIntExW(Value, FALSE, 8, StrValue, ARRAYSIZE(StrValue))
#define Str_OctFromIntA(Value, StrValue) Str_FromIntExA(Value, FALSE, 8, StrValue, ARRAYSIZE(StrValue))
#define Str_OctFromUIntW(Value, StrValue) Str_FromIntExW(Value, TRUE, 8, StrValue, ARRAYSIZE(StrValue))
#define Str_OctFromUIntA(Value, StrValue) Str_FromIntExA(Value, TRUE, 8, StrValue, ARRAYSIZE(StrValue))
#define Str_BinFromIntW(Value, StrValue) Str_FromIntExW(Value, FALSE, 2, StrValue, ARRAYSIZE(StrValue))
#define Str_BinFromIntA(Value, StrValue) Str_FromIntExA(Value, FALSE, 2, StrValue, ARRAYSIZE(StrValue))
#define Str_BinFromUIntW(Value, StrValue) Str_FromIntExW(Value, TRUE, 2, StrValue, ARRAYSIZE(StrValue))
#define Str_BinFromUIntA(Value, StrValue) Str_FromIntExA(Value, TRUE, 2, StrValue, ARRAYSIZE(StrValue))

#pragma endregion

EXTERN_C_END
