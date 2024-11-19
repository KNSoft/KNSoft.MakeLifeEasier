#include "../MakeLifeEasier.inl"

template <typename TChar>
static
VOID
ParseCmdline(
    _In_ _Null_terminated_ CONST TChar* Cmdline,
    _Outptr_opt_result_z_ TChar** ArgV,
    _Out_opt_ TChar* ArgPtr,
    _Out_ PULONG ArgC,
    _Out_ PULONG CharC)
{
    CONST TChar* p;
    TChar c;
    BOOL IsQuoted, DoCopy;
    ULONG SlashCount;

    *CharC = 0;
    *ArgC = 1;

    p = Cmdline;
    if (ArgV)
    {
        *ArgV++ = ArgPtr;
    }

    IsQuoted = FALSE;
    do
    {
        if (*p == '"')
        {
            IsQuoted = !IsQuoted;
            c = *p++;
            continue;
        }

        ++ * CharC;
        if (ArgPtr)
        {
            *ArgPtr++ = *p;
        }

        c = *p++;
    } while (c != '\0' && (IsQuoted || (c != ' ' && c != '\t')));

    if (c == '\0')
    {
        p--;
    } else
    {
        if (ArgPtr)
        {
            *(ArgPtr - 1) = '\0';
        }
    }

    IsQuoted = FALSE;

    for (;;)
    {
        if (*p)
        {
            while (*p == ' ' || *p == '\t')
            {
                ++p;
            }
        }
        if (*p == '\0')
        {
            break;
        }
        if (ArgV)
        {
            *ArgV++ = ArgPtr;
        }
        ++ * ArgC;

        for (;;)
        {
            DoCopy = TRUE;
            SlashCount = 0;

            while (*p == '\\')
            {
                ++p;
                ++SlashCount;
            }

            if (*p == '"')
            {
                if (SlashCount % 2 == 0)
                {
                    if (IsQuoted && p[1] == '"')
                    {
                        p++;
                    } else
                    {
                        DoCopy = FALSE;
                        IsQuoted = !IsQuoted;
                    }
                }

                SlashCount /= 2;
            }

            while (SlashCount--)
            {
                if (ArgPtr)
                {
                    *ArgPtr++ = '\\';
                }
                ++ * CharC;
            }

            if (*p == '\0' || (!IsQuoted && (*p == ' ' || *p == '\t')))
            {
                break;
            }

            if (DoCopy)
            {
                if (ArgPtr)
                    *ArgPtr++ = *p;

                ++*CharC;
            }

            ++p;
        }

        if (ArgPtr)
        {
            *ArgPtr++ = '\0';
        }

        ++ * CharC;
    }

    if (ArgV)
    {
        *ArgV++ = NULL;
    }

    ++ * ArgC;
}

template <typename TChar>
static
NTSTATUS
ParseCmdlineToArgV(
    _In_ _Null_terminated_ CONST TChar* Cmdline,
    _Out_ PULONG ArgC,
    _Outptr_result_z_ TChar*** ArgV)
{
    ULONG ArgCount, CchCmdline, ArgPtrSize;
    PVOID Buffer;

    ParseCmdline<TChar>(Cmdline, NULL, NULL, &ArgCount, &CchCmdline);
    ArgPtrSize = ArgCount * sizeof(PVOID);

    Buffer = Mem_Alloc(ArgPtrSize + CchCmdline * sizeof(TChar));
    if (Buffer == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    ParseCmdline<TChar>(Cmdline,
                        reinterpret_cast<_Null_terminated_ TChar**>(Buffer),
                        reinterpret_cast<TChar*>(Add2Ptr(Buffer, ArgPtrSize)),
                        &ArgCount,
                        &CchCmdline);

    *ArgC = ArgCount - 1;
    *ArgV = reinterpret_cast<_Null_terminated_ TChar**>(Buffer);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PS_CommandLineToArgvW(
    _In_ PCWSTR Cmdline,
    _Out_ PULONG ArgC,
    _Outptr_result_z_ PWSTR** ArgV)
{
    NTSTATUS Status = ParseCmdlineToArgV<WCHAR>(Cmdline, ArgC, ArgV);
    _Analysis_assume_nullterminated_(**ArgV);
    return Status;
}

NTSTATUS
NTAPI
PS_CommandLineToArgvA(
    _In_ PCSTR Cmdline,
    _Out_ PULONG ArgC,
    _Outptr_result_z_ PSTR** ArgV)
{
    NTSTATUS Status = ParseCmdlineToArgV<CHAR>(Cmdline, ArgC, ArgV);
    _Analysis_assume_nullterminated_(**ArgV);
    return Status;
}
