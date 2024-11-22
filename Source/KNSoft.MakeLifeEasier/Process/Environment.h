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

FORCEINLINE
_Success_(return != FALSE)
LOGICAL
PS_FreeCommandLineArgv(
    __drv_freesMem(Mem) _Frees_ptr_ _Post_invalid_ PVOID ArgV)
{
    return Mem_Free(ArgV);
}

EXTERN_C_END
