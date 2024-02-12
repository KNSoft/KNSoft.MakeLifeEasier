#include "../CRT.inl"
#include "../Memory/Memory.inl"
#include "../../Include/NTAssassin/NT/Macro.h"

EXTERN_C_START

INT __argc = 0;
CHAR** __argv = NULL;
WCHAR** __wargv = NULL;
CHAR* _acmdln = NULL;
WCHAR* _wcmdln = NULL;

EXTERN_C_END

template <typename TChar>
static VOID IntParseCmdline(
    _In_z_ TChar* Cmdline,
    _Out_opt_ TChar** ArgV,
    _Out_opt_ TChar* ArgPtr,
    _Out_ PULONG ArgC,
    _Out_ PSIZE_T CharC)
{
    TChar* p;
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
static NTSTATUS IntCmdlineToArgV(_In_z_ TChar* Cmdline, _Out_ PULONG ArgC, _Out_ TChar*** ArgV)
{
    ULONG ArgCount;
    SIZE_T CchCmdline, ArgPtrSize;
    PVOID Buffer;

    IntParseCmdline<TChar>(Cmdline, NULL, NULL, &ArgCount, &CchCmdline);
    ArgPtrSize = ArgCount * sizeof(PVOID);

    Buffer = HeapMemAllocate(ArgPtrSize + CchCmdline * sizeof(TChar), 0);
    if (Buffer == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    IntParseCmdline<TChar>(Cmdline,
                           reinterpret_cast<TChar**>(Buffer),
                           reinterpret_cast<TChar*>(Add2Ptr(Buffer, ArgPtrSize)),
                           &ArgCount,
                           &CchCmdline);

    *ArgC = ArgCount - 1;
    *ArgV = reinterpret_cast<TChar**>(Buffer);
    return STATUS_SUCCESS;
}

static PUNICODE_STRING g_Cmdline = NULL;

static VOID IntInitCmdline()
{
    PUNICODE_STRING Cmdline;

    if (g_Cmdline == NULL)
    {
        Cmdline = &NtCurrentPeb()->ProcessParameters->CommandLine;
        if (Cmdline->Buffer != NULL &&
            Cmdline->Length > 0 &&
            Cmdline->MaximumLength > Cmdline->Length &&
            Cmdline->Buffer[Cmdline->Length / sizeof(WCHAR)] == UNICODE_NULL)
        {
            g_Cmdline = Cmdline;
        }
    }
}

EXTERN_C_START

VOID Startup_InitCmdlineW()
{
    if (_wcmdln == NULL)
    {
        IntInitCmdline();
        if (g_Cmdline != NULL)
        {
            _wcmdln = g_Cmdline->Buffer;
        }
    }
}

static ANSI_STRING g_CmdlineA = { 0 };

VOID Startup_InitCmdlineA()
{
    if (_acmdln == NULL)
    {
        IntInitCmdline();
        if (g_Cmdline != NULL && NT_SUCCESS(RtlUnicodeStringToAnsiString(&g_CmdlineA, g_Cmdline, TRUE)))
        {
            _acmdln = g_CmdlineA.Buffer;
        }
    }
}

VOID Startup_InitCmdlineArgW()
{
    ULONG ArgC;
    PWSTR* ArgV;

    if (__wargv == NULL)
    {
        Startup_InitCmdlineW();
        if (_wcmdln != NULL && NT_SUCCESS(IntCmdlineToArgV<WCHAR>(_wcmdln, &ArgC, &ArgV)))
        {
            __argc = ArgC;
            __wargv = ArgV;
        }
    }
}

VOID Startup_InitCmdlineArgA()
{
    ULONG ArgC;
    PSTR* ArgV;

    if (__argv == NULL)
    {
        Startup_InitCmdlineA();
        if (_acmdln != NULL && NT_SUCCESS(IntCmdlineToArgV<CHAR>(_acmdln, &ArgC, &ArgV)))
        {
            __argc = ArgC;
            __argv = ArgV;
        }
    }
}

VOID Startup_FreeCmdlineA()
{
    RtlFreeAnsiString(&g_CmdlineA);
}

VOID Startup_FreeCmdlineArgW()
{
    if (__wargv != NULL)
    {
        HeapMemFree(__wargv);
        __wargv = NULL;
        __argc = 0;
    }
}

VOID Startup_FreeCmdlineArgA()
{
    if (__argv != NULL)
    {
        HeapMemFree(__argv);
        __argv = NULL;
        __argc = 0;
    }
}

EXTERN_C_END
