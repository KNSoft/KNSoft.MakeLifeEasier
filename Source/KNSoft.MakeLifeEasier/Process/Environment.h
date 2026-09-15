#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

MLE_API
NTSTATUS
NTAPI
PS_CommandLineToArgvW(
    _In_ PCWSTR Cmdline,
    _Out_ PULONG ArgC,
    _Outptr_result_z_ PWSTR** ArgV);

MLE_API
NTSTATUS
NTAPI
PS_CommandLineToArgvA(
    _In_ PCSTR Cmdline,
    _Out_ PULONG ArgC,
    _Outptr_result_z_ PSTR** ArgV);

MLE_API
NTSTATUS
NTAPI
PS_ArgvToCommandLineW(
    _In_ ULONG ArgC,
    _In_reads_(ArgC) _At_buffer_(ArgV, _Iter_, ArgC, _In_) PCWSTR const* ArgV,
    _Outptr_ PWSTR* Cmdline);

MLE_API
NTSTATUS
NTAPI
PS_ArgvToCommandLineA(
    _In_ ULONG ArgC,
    _In_reads_(ArgC) _At_buffer_(ArgV, _Iter_, ArgC, _In_) PCSTR const* ArgV,
    _Outptr_ PSTR* Cmdline);

FORCEINLINE
_Success_(return != FALSE)
LOGICAL
PS_FreeCommandLineBuffer(
    __drv_freesMem(Mem) _Frees_ptr_ _Post_invalid_ PVOID Buffer)
{
    return Mem_Free(Buffer);
}

EXTERN_C_END
