#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

FORCEINLINE
_Success_(return != NULL)
HANDLE
IO_ConGetStdInput(VOID)
{
    HANDLE StdInput;

    if (IS_NT_VERSION_GE(NT_VERSION_VISTA) && NtCurrentPeb()->ProcessParameters->WindowFlags & STARTF_USEHOTKEY)
    {
        return NULL;
    }
    StdInput = NtCurrentPeb()->ProcessParameters->StandardInput;
    return StdInput == INVALID_HANDLE_VALUE ? NULL : StdInput;
}

FORCEINLINE
_Success_(return != NULL)
HANDLE
IO_ConGetStdOutput(VOID)
{
    HANDLE StdOutput;

    if (IS_NT_VERSION_GE(NT_VERSION_VISTA) && NtCurrentPeb()->ProcessParameters->WindowFlags & STARTF_USEMONITOR)
    {
        return NULL;
    }
    StdOutput = NtCurrentPeb()->ProcessParameters->StandardOutput;
    return StdOutput == INVALID_HANDLE_VALUE ? NULL : StdOutput;
}

FORCEINLINE
_Success_(return != NULL)
HANDLE
IO_ConGetStdError(VOID)
{
    HANDLE StdError;

    StdError = NtCurrentPeb()->ProcessParameters->StandardError;
    return StdError == INVALID_HANDLE_VALUE ? NULL : StdError;
}

MLE_API
VOID
NTAPI
IO_ConPrintEx(
    _In_reads_bytes_(TextSize) PCCH Text,
    _In_ ULONG TextSize);

MLE_API
VOID
_cdecl
IO_ConPrintFV(
    _In_ _Printf_format_string_ PCSTR Format,
    _In_opt_ va_list ArgList);

MLE_API
VOID
_cdecl
IO_ConPrintF(
    _In_ _Printf_format_string_ PCSTR Format,
    ...);

EXTERN_C_END
