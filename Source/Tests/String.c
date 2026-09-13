#include "Test.h"

static const UNICODE_STRING g_usSampleString = RTL_CONSTANT_STRING(L"KNSoft.MakeLifeEasier");

TEST_FUNC(String_Hash)
{
    NTSTATUS Status;
    ULONG HashX65599;

    Status = RtlHashUnicodeString((PUNICODE_STRING)&g_usSampleString,
                                  FALSE,
                                  HASH_STRING_ALGORITHM_X65599,
                                  &HashX65599);
    if (NT_SUCCESS(Status))
    {
        TEST_OK(Str_Hash_X65599W(g_usSampleString.Buffer) == HashX65599);
    } else
    {
        TEST_SKIP("RtlHashUnicodeString failed with 0x%08lX\n", Status);
    }
}
