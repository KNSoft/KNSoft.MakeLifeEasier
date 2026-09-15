#include "../MakeLifeEasier.inl"

#ifndef ARGPARSE_ALLOC_FUNCNAME

#define TChar WCHAR
#include <KNSoft/NDK/Package/ArgParse.inl>
#undef TChar

#define TChar CHAR
#include <KNSoft/NDK/Package/ArgParse.inl>
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

NTSTATUS
NTAPI
PS_ArgvToCommandLineW(
    _In_ ULONG ArgC,
    _In_reads_(ArgC) _At_buffer_(ArgV, _Iter_, ArgC, _In_) PCWSTR const* ArgV,
    _Outptr_ PWSTR* Cmdline)
{
    return ARGPARSE_ALLOC_CMDLINE_FUNCNAME(WCHAR)(ArgC, ArgV, Cmdline);
}

NTSTATUS
NTAPI
PS_ArgvToCommandLineA(
    _In_ ULONG ArgC,
    _In_reads_(ArgC) _At_buffer_(ArgV, _Iter_, ArgC, _In_) PCSTR const* ArgV,
    _Outptr_ PSTR* Cmdline)
{
    return ARGPARSE_ALLOC_CMDLINE_FUNCNAME(CHAR)(ArgC, ArgV, Cmdline);
}

EXTERN_C_END
