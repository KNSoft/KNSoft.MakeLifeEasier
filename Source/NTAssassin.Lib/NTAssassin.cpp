#include "NTAssassin.Lib.inl"

BOOL _NTA_Initialized = NTA_Initialize();
HANDLE _NTA_Heap;
CPUID_INFO _NTA_CpuidInfo_00_00;
CPUID_INFO _NTA_CpuidInfo_01_00;
CPUID_INFO _NTA_CpuidInfo_07_00;
CPUID_INFO _NTA_CpuidInfo_07_01;
unsigned __int64 _NTA_XCR_XFeature;
PPEB _NTA_PEB;
USHORT _NTA_Process_WinntVersion;

BOOL NTAPI NTA_Initialize()
{
    /* Detect CPU features */
    __cpuid(_NTA_CpuidInfo_00_00.Registers, 0);
    __cpuid(_NTA_CpuidInfo_01_00.Registers, 1);

    if (_NTA_CpuidInfo_01_00.F01_00.FeatureInfo.OSXSAVE)
    {
        _NTA_XCR_XFeature = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
    }

    if (_NTA_CpuidInfo_00_00.F00_00.MaxInputValue >= 7)
    {
        __cpuid(_NTA_CpuidInfo_07_00.Registers, 7);
        __cpuidex(_NTA_CpuidInfo_07_01.Registers, 7, 1);
    }

    /* Process environment */
    _NTA_Heap = NtGetProcessHeap();
    _NTA_PEB = NtCurrentPeb();
    _NTA_Process_WinntVersion = ((_NTA_PEB->OSMajorVersion & 0xFF) << 8) | (_NTA_PEB->OSMinorVersion & 0xFF);

    return TRUE;
}
