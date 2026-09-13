#include "../MakeLifeEasier.inl"

VOID
NTAPI
IO_ConPrintEx(
    _In_reads_bytes_(TextSize) PCCH Text,
    _In_ ULONG TextSize)
{
    HANDLE StdOutput = IO_ConGetStdOutput();

    if (StdOutput != NULL)
    {
        IO_WriteFile(StdOutput, NULL, (PVOID)Text, TextSize, NULL);
    }
}

VOID
_cdecl
IO_ConPrintFV(
    _In_ _Printf_format_string_ PCSTR Format,
    _In_opt_ va_list ArgList)
{
    CHAR Buffer[512 + 1];  // Same limitation as DbgPrint
    ULONG Length, NewLength;
    HANDLE StdOutput;
    PSTR Text;

    /* Write standard output if exists */
    StdOutput = IO_ConGetStdOutput();
    if (StdOutput == NULL)
    {
        return;
    }

    /* Format string */
    Length = StrSafe_CchVPrintfA(Buffer, ARRAYSIZE(Buffer), Format, ArgList);
    if (Length == 0)
    {
        return;
    }

    /* Allocate buffer if Buffer too small */
    if (Length >= ARRAYSIZE(Buffer))
    {
        Text = Mem_Alloc((SIZE_T)Length + 1);
        if (Text != NULL)
        {
            NewLength = StrSafe_CchVPrintfA(Text, (SIZE_T)Length + 1, Format, ArgList);
            if (NewLength > 0 && NewLength < Length)
            {
                Length = NewLength;
                goto _Print_Stdout;
            }
            Mem_Free(Text);
        }

        /* New allocated buffer unavailable, fallback to Buffer (truncated) */
        Length = ARRAYSIZE(Buffer) - 1;
    }
    Text = Buffer;

_Print_Stdout:
    IO_WriteFile(StdOutput, NULL, Text, Length, NULL);
    if (Text != Buffer)
    {
        Mem_Free(Text);
    }
}

VOID
_cdecl
IO_ConPrintF(
    _In_ _Printf_format_string_ PCSTR Format,
    ...)
{
    va_list ArgList;

    va_start(ArgList, Format);
    IO_ConPrintFV(Format, ArgList);
    va_end(ArgList);
}
