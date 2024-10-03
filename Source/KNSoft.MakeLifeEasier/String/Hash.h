#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

MLE_API
ULONG
NTAPI
Str_Hash_X65599W(
    _In_z_ PCWSTR String);

MLE_API
ULONG
NTAPI
Str_Hash_X65599A(
    _In_z_ PCSTR String);

MLE_API
ULONG
NTAPI
Str_Hash_FNV1aW(
    _In_z_ PCWSTR String);

MLE_API
ULONG
NTAPI
Str_Hash_FNV1aA(
    _In_z_ PCSTR String);

EXTERN_C_END
