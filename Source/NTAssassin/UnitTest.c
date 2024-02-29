#include "../Include/NTAssassin/NTAssassin/UnitTest.h"

#include "../Include/NTAssassin/WinDef/API/Ntdll.h"

#pragma section(".NUT$ELA", long, read) // First
#pragma section(".NUT$ELZ", long, read) // Last

__declspec(allocate(".NUT$ELA")) static PUNITTEST_ENTRY g_EntryList_First = NULL;
__declspec(allocate(".NUT$ELZ")) static PUNITTEST_ENTRY g_EntryList_Last= NULL;

#pragma comment(linker, "/merge:.NUT=.rdata")

BOOL NTAPI UnitTest_EnumEntries(_In_ __callback FN_UNITTEST_ENUM_PROC* Callback, _In_opt_ PVOID Context)
{
    PUNITTEST_ENTRY* Entry;
    BOOL Ret;

    for (Entry = &g_EntryList_First; Entry != &g_EntryList_Last; Entry++)
    {
        if (*Entry != NULL)
        {
            Ret = Callback(*Entry, Context);
            if (!Ret)
            {
                return Ret;
            }
        }
    }

    return TRUE;
}

_Ret_maybenull_
_Must_inspect_result_
PUNITTEST_ENTRY NTAPI UnitTest_FindEntry(_In_z_ PCWSTR Name)
{
    PUNITTEST_ENTRY* Entry;
    UNICODE_STRING NameString;

    RtlInitUnicodeString(&NameString, Name);

    for (Entry = &g_EntryList_First; Entry != &g_EntryList_Last; Entry++)
    {
        if (*Entry != NULL && RtlEqualUnicodeString(&(*Entry)->Name, &NameString, FALSE))
        {
            return *Entry;
        }
    }

    return NULL;
}

ULONG NTAPI UnitTest_RunAll(_Out_ PUNITTEST_RESULT Result)
{
    ULONG Ret = 0;
    PUNITTEST_ENTRY* Entry;

    RtlZeroMemory(Result, sizeof(*Result));

    for (Entry = &g_EntryList_First; Entry != &g_EntryList_Last; Entry++)
    {
        if (*Entry != NULL)
        {
            (*Entry)->Proc(Result);
            Ret++;
        }
    }

    return Ret;
}

_Success_(return != FALSE)
BOOL NTAPI UnitTest_Run(_In_z_ PCWSTR Name, _Out_ PUNITTEST_RESULT Result)
{
    PUNITTEST_ENTRY Entry = UnitTest_FindEntry(Name);

    if (Entry == NULL)
    {
        return FALSE;
    }

    Entry->Proc(Result);
    return TRUE;
}

_Success_(return > 0)
ULONG NTAPI UnitTest_Main(_In_ int argc, _In_reads_(argc) _Pre_z_ wchar_t** argv, _Out_ PUNITTEST_RESULT Result)
{
    ULONG RunCount;
    int i;
    UNICODE_STRING NameString;
    PUNITTEST_ENTRY* Entry;

    if (argc == 1)
    {
        RunCount = UnitTest_RunAll(Result);
    } else if (argc > 1)
    {
        RtlZeroMemory(Result, sizeof(*Result));
        RunCount = 0;

        for (i = 1; i < argc; i++)
        {
            RtlInitUnicodeString(&NameString, argv[i]);

            for (Entry = &g_EntryList_First; Entry != &g_EntryList_Last; Entry++)
            {
                if (RtlEqualUnicodeString(&(*Entry)->Name, &NameString, FALSE))
                {
                    (*Entry)->Proc(Result);
                    RunCount++;
                    break;
                }
            }
        }
    } else
    {
        RunCount = 0;
    }

    return RunCount;
}
