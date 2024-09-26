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

#ifdef UNICODE
#define Str_Hash_X65599 Str_Hash_X65599W
#define Str_Hash_FNV1a Str_Hash_FNV1aW
#else
#define Str_Hash_X65599 Str_Hash_X65599A
#define Str_Hash_FNV1a Str_Hash_FNV1aA
#endif
#define Str_Hash Str_Hash_X65599

EXTERN_C_END
