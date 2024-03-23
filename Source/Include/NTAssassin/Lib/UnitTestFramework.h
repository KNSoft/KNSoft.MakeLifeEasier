#pragma once

#include "../NDK/NT/MinDef.h"

typedef struct _UNITTEST_RESULT
{
    ULONG Pass;
    ULONG Skip;
    ULONG Fail;
} UNITTEST_RESULT, *PUNITTEST_RESULT;

typedef VOID NTAPI FN_UNITTEST_PROC(UNITTEST_RESULT* _NTAssassin_UnitTest_Result);

typedef struct _UNITTEST_ENTRY
{
    FN_UNITTEST_PROC* Proc;
    UNICODE_STRING Name;
} UNITTEST_ENTRY, *PUNITTEST_ENTRY;

typedef BOOL CALLBACK FN_UNITTEST_ENUM_PROC(_In_ PUNITTEST_ENTRY Entry, _In_opt_ PVOID Context);

EXTERN_C_START

NTA_API
BOOL NTAPI UnitTest_EnumEntries(
    _In_ __callback FN_UNITTEST_ENUM_PROC* Callback,
    _In_opt_ PVOID Context);

NTA_API
_Ret_maybenull_
_Must_inspect_result_
PUNITTEST_ENTRY NTAPI UnitTest_FindEntry(
    _In_z_ PCWSTR Name);

NTA_API
ULONG NTAPI UnitTest_RunAll(
    _Out_ PUNITTEST_RESULT Result);

NTA_API
_Success_(return != FALSE)
BOOL NTAPI UnitTest_Run(
    _In_z_ PCWSTR Name,
    _Out_ PUNITTEST_RESULT Result);

NTA_API
_Success_(return > 0)
ULONG NTAPI UnitTest_Main(
    _In_ int argc,
    _In_reads_(argc) _Pre_z_ wchar_t** argv,
    _Out_ PUNITTEST_RESULT Result);

EXTERN_C_END

#pragma section(".NUT$ELB", long, read)

#define TEST_DECL(Name)\
VOID NTAPI Name(UNITTEST_RESULT* _NTAssassin_UnitTest_Result);\
static UNITTEST_ENTRY _NTAssassin_UnitTest_Entry_##Name = { Name, RTL_CONSTANT_STRING(L###Name) };\
__declspec(allocate(".NUT$ELB")) PUNITTEST_ENTRY _NTAssassin_UnitTest_Entry_Ptr_##Name = &_NTAssassin_UnitTest_Entry_##Name;\
static VOID NTAPI Name(UNITTEST_RESULT* _NTAssassin_UnitTest_Result)

#define TEST_RESULT(r) (_NTAssassin_UnitTest_Result->r++)
#define TEST_PASS() TEST_RESULT(Pass)
#define TEST_SKIP() TEST_RESULT(Skip)
#define TEST_FAIL() TEST_RESULT(Fail)
