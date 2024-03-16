#include "Test.h"

#include <stdio.h>
#include <dpfilter.h>

#pragma region CRT Error Mode

#if !defined(_VC_NODEFAULTLIB)

#include <stdlib.h>
#include <crtdbg.h>

static _invalid_parameter_handler g_pfnOldHandler = NULL;
static int g_iReportMode;

void __cdecl _invalid_parameter_nothrow(
   const wchar_t* expression,
   const wchar_t* function,
   const wchar_t* file,
   unsigned int line,
   uintptr_t pReserved)
{
    DbgPrint("Invalid parameter passed to C runtime function.\n");
}

#endif

VOID SetNtdllCRTErrorMode()
{
#if !defined(_VC_NODEFAULTLIB)
    g_pfnOldHandler = _set_invalid_parameter_handler(_invalid_parameter_nothrow);
    g_iReportMode = _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
#endif
}

VOID RevertCRTErrorMode()
{
#if !defined(_VC_NODEFAULTLIB)
    if (g_iReportMode != -1)
    {
        _CrtSetReportMode(_CRT_ASSERT, g_iReportMode);
    }
    _set_invalid_parameter_handler(g_pfnOldHandler);
#endif
}

#pragma endregion

#pragma region Print

VOID PrintF(_In_z_ _Printf_format_string_ PCSTR Format, ...)
{
    CHAR szTemp[512];
    va_list argList;
    ULONG uLen;
    HANDLE hStdOut;
    IO_STATUS_BLOCK IoStatusBlock;

    va_start(argList, Format);
    vDbgPrintEx(MAXULONG, DPFLTR_ERROR_LEVEL, Format, argList);
    
    hStdOut = NtCurrentPeb()->ProcessParameters->StandardOutput;
    if (hStdOut != NULL)
    {
        uLen = String_CchVPrintfA(szTemp, ARRAYSIZE(szTemp), Format, argList);
        if (uLen > 0 && uLen < ARRAYSIZE(szTemp))
        {
            NtWriteFile(hStdOut, NULL, NULL, NULL, &IoStatusBlock, szTemp, uLen, NULL, NULL);
        }
    }
}

static PCSTR g_szDividingLine = "\n**********************************************************************\n";

VOID PrintTitle(_In_z_ PCSTR Name, _In_z_ PCSTR Description)
{
    PrintF(g_szDividingLine);
    PrintF("%hs\n\t%hs", Name, Description);
    PrintF(g_szDividingLine);
}

#pragma endregion
