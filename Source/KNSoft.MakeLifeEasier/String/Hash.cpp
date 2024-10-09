#include "../MakeLifeEasier.inl"

#pragma region X65599

template <typename TChar>
static
ULONG
Str_Hash_X65599_Impl(
    _In_z_ CONST TChar* String)
{
    ULONG Hash = 0;
    CONST TChar* psz;

    for (psz = String; *psz != (TChar)'\0'; psz++)
    {
        Hash = 65599 * Hash + *psz;
    }

    return Hash;
}

ULONG
NTAPI
Str_Hash_X65599W(
    _In_ PCWSTR String)
{
    return Str_Hash_X65599_Impl<WCHAR>(String);
}

ULONG
NTAPI
Str_Hash_X65599A(
    _In_ PCSTR String)
{
    return Str_Hash_X65599_Impl<CHAR>(String);
}

#pragma endregion

#pragma region FNV1a

template <typename TChar>
static
ULONG
Str_Hash_FNV1a_Impl(
    _In_z_ CONST TChar* String)
{
    ULONG Hash = 2166136261U;
    CONST TChar* psz;

    for (psz = String; *psz != (TChar)'\0'; psz++)
    {
        Hash ^= *psz;
        Hash *= 16777619U;
    }

    return Hash;
}

ULONG
NTAPI
Str_Hash_FNV1aW(
    _In_ PCWSTR String)
{
    return Str_Hash_FNV1a_Impl<WCHAR>(String);
}

ULONG
NTAPI
Str_Hash_FNV1aA(
    _In_ PCSTR String)
{
    return Str_Hash_FNV1a_Impl<CHAR>(String);
}

#pragma endregion
