#define MLE_API

#include "Test.h"

#include <KNSoft/NDK/Package/UnitTest.inl>

#pragma comment (lib, "KNSoft.MakeLifeEasier.lib")

TEST_DECL_FUNC(Math_Round);
TEST_DECL_FUNC(String_Hash);
TEST_DECL_FUNC(RegistryWin32);
TEST_DECL_FUNC(RegistryNT);

TEST_DECL_FUNC(CreateSuperToken);

CONST UNITTEST_ENTRY UnitTestList[] = {
    TEST_DECL_ENTRY(Math_Round),
    TEST_DECL_ENTRY(String_Hash),
    TEST_DECL_ENTRY(RegistryWin32),
    TEST_DECL_ENTRY(RegistryNT),

    TEST_DECL_MANUAL_ENTRY(CreateSuperToken),
    { 0 }
};

int
_cdecl
wmain(
    _In_ int argc,
    _In_reads_(argc) _Pre_z_ wchar_t** argv)
{
    PVOID p1 = NULL, p2 = INVALID_HANDLE_VALUE, p3;
    p3 = _InterlockedReadPointer((const volatile void**)&p2);
    ANSI_STRING a = RTL_CONSTANT_STRING("NtWow64GetNativeSystemInformation");
    /*
    SYSTEM_PROCESSOR_INFORMATION spi;
    PVOID NtdllBase = (CONTAINING_RECORD(NtCurrentPeb()->Ldr->InInitializationOrderModuleList.Flink, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks)->DllBase);
    typeof(NtWow64GetNativeSystemInformation)* pc;

    LdrGetProcedureAddress(NtdllBase, &a, 0, (PVOID*)&pc);
    pc(SystemProcessorInformation, &spi, sizeof(spi), NULL)*/;
    PVOID p;

    p = NtReadTebPVOID(WOW32Reserved);
    p = NtReadTebPVOID(WowTebOffset);
    return UnitTest_Main(argc, argv);
}
