#define MLE_API

#include "Test.h"

#include <KNSoft/NDK/Package/UnitTest.inl>

#pragma comment (lib, "KNSoft.MakeLifeEasier.lib")

TEST_DECL_FUNC(Math_Round);
TEST_DECL_FUNC(String_Hash);
TEST_DECL_FUNC(RegistryWin32);
TEST_DECL_FUNC(RegistryNT);

CONST UNITTEST_ENTRY UnitTestList[] = {
    TEST_DECL_ENTRY(Math_Round),
    TEST_DECL_ENTRY(String_Hash),
    TEST_DECL_ENTRY(RegistryWin32),
    TEST_DECL_ENTRY(RegistryNT),
    { 0 }
};

int
_cdecl
wmain(
    _In_ int argc,
    _In_reads_(argc) _Pre_z_ wchar_t** argv)
{
    return UnitTest_Main(argc, argv);
}
