#define MLE_API

#include "Test.h"

#include <KNSoft/NDK/Package/UnitTest.inl>

#pragma comment (lib, "KNSoft.MakeLifeEasier.lib")

TEST_DECL_FUNC(Math_Round);
TEST_DECL_FUNC(String_Hash);

CONST UNITTEST_ENTRY UnitTestList[] = {
    TEST_DECL_ENTRY(Math_Round),
    TEST_DECL_ENTRY(String_Hash),
    { 0 }
};

int
_cdecl
wmain(
    _In_ int argc,
    _In_reads_(argc) _Pre_z_ wchar_t** argv)
{
    RECT rc = { 1, 2, 3, 4 };
    W32ERROR Ret;
    do
    {
        Ret = Dlg_RectEditor(NULL, &rc);
    } while (Ret == ERROR_SUCCESS);

    return UnitTest_Main(argc, argv);
}
