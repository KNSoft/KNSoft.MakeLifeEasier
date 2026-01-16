#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

MLE_API
ULONG
NTAPI
Str_Hash_X65599W(
    _In_ PCWSTR String);

MLE_API
ULONG
NTAPI
Str_Hash_X65599A(
    _In_ PCSTR String);

MLE_API
ULONG
NTAPI
Str_Hash_FNV1aW(
    _In_ PCWSTR String);

MLE_API
ULONG
NTAPI
Str_Hash_FNV1aA(
    _In_ PCSTR String);

EXTERN_C_END
