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

TEST_FUNC(String_Case)
{
    CHAR Ansi[] = "Hello, World! 123";
    WCHAR Wide[] = L"Hello, World! 123";
    CHAR EmptyA[] = "";
    WCHAR EmptyW[] = L"";

    Str_UpperA(Ansi);
    TEST_OK(Str_EqualA(Ansi, "HELLO, WORLD! 123"));
    Str_LowerA(Ansi);
    TEST_OK(Str_EqualA(Ansi, "hello, world! 123"));
    Str_UpperW(Wide);
    TEST_OK(Str_EqualW(Wide, L"HELLO, WORLD! 123"));
    Str_LowerW(Wide);
    TEST_OK(Str_EqualW(Wide, L"hello, world! 123"));

    TEST_OK(Str_UpperCharA('a') == 'A' && Str_UpperCharA('A') == 'A' && Str_UpperCharA('0') == '0');
    TEST_OK(Str_LowerCharA('Z') == 'z' && Str_LowerCharA('z') == 'z' && Str_LowerCharA('0') == '0');
    TEST_OK(Str_UpperCharW(L'a') == L'A' && Str_LowerCharW(L'Z') == L'z');
    TEST_OK(Str_UpperCharA((CHAR)0xE9) == (CHAR)0xE9 && Str_LowerCharA((CHAR)0xC9) == (CHAR)0xC9);
    TEST_OK(Str_UpperCharW(L'\x00E9') == L'\x00C9' && Str_LowerCharW(L'\x00C9') == L'\x00E9');

    TEST_OK(Str_EqualIA("Abc123", "aBC123"));
    TEST_OK(!Str_EqualIA("Abc123", "aBC124"));
    TEST_OK(Str_EqualIW(L"Abc123", L"aBC123"));
    TEST_OK(!Str_EqualIW(L"Abc123", L"aBC124"));

    TEST_OK(Str_StrIA(EmptyA, "") == EmptyA);
    TEST_OK(Str_StrIW(EmptyW, L"") == EmptyW);
    TEST_OK(Str_StrIA(Ansi, "") == Ansi);
    TEST_OK(Str_StrIW(Wide, L"") == Wide);
    TEST_OK(Str_StrIA(EmptyA, "a") == NULL);
    TEST_OK(Str_StrIW(EmptyW, L"a") == NULL);

    TEST_OK(Str_StrIA("Hello, World!", "WORLD") != NULL);
    TEST_OK(Str_StrIA("Hello, World!", "xyz") == NULL);
    TEST_OK(Str_StrIA("Hello", "Hello, World!") == NULL);
    TEST_OK(Str_StrIW(L"Hello, World!", L"wOrLd") != NULL);
    TEST_OK(Str_StrIW(L"Hello, World!", L"xyz") == NULL);
    TEST_OK(Str_StrIW(L"Hello", L"Hello, World!") == NULL);
}
