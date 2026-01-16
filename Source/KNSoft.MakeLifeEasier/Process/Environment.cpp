#include "../MakeLifeEasier.inl"

#ifndef ARGPARSE_ALLOC_FUNCNAME

#define TChar WCHAR
#include <KNSoft/NDK/Package/ArgvParsing.inl>
#undef TChar

#define TChar CHAR
#include <KNSoft/NDK/Package/ArgvParsing.inl>
#undef TChar

#endif

EXTERN_C_START

NTSTATUS
NTAPI
PS_CommandLineToArgvW(
    _In_ PCWSTR Cmdline,
    _Out_ PULONG ArgC,
    _Outptr_result_z_ PWSTR** ArgV)
{
    return ARGPARSE_ALLOC_FUNCNAME(WCHAR)(Cmdline, ArgC, ArgV);
}

NTSTATUS
NTAPI
PS_CommandLineToArgvA(
    _In_ PCSTR Cmdline,
    _Out_ PULONG ArgC,
    _Outptr_result_z_ PSTR** ArgV)
{
    return ARGPARSE_ALLOC_FUNCNAME(CHAR)(Cmdline, ArgC, ArgV);
}

EXTERN_C_END
