#include "Startup.inl"

#include <isa_availability.h>

#if defined(_M_IX86)

int __isa_available = __ISA_AVAILABLE_X86;
int __isa_enabled = 0;
int __favor = 0;

int __isa_available_init()
{
    CPUID_INFO CpuInfo01, CpuInfo7;
    UINT CpuIdMaxInputValue;
    BOOLEAN IsSSE2Enabled, IsGenuineIntel;
    ULONGLONG XFeatureMask;

    __isa_available = __ISA_AVAILABLE_X86;
    __isa_enabled |= (1 << __ISA_AVAILABLE_X86);

    IsSSE2Enabled = SharedUserData->ProcessorFeatures[PF_XMMI64_INSTRUCTIONS_AVAILABLE];
    if (!IsSSE2Enabled)
    {
        return 0;
    }

    __cpuid(CpuInfo01.Registers, 0);
    CpuIdMaxInputValue = CpuInfo01.F00_00.MaxInputValue;
    IsGenuineIntel = (CpuInfo01.F00_00.VendorIdString[0] == ('uneG') &&
                      CpuInfo01.F00_00.VendorIdString[1] == ('Ieni') &&
                      CpuInfo01.F00_00.VendorIdString[2] == ('letn'));

    __cpuid(CpuInfo01.Registers, 0);
    CpuInfo01.Eax &= CPUID_INTEL_VERSION_INFO_MASK;

    if (IsGenuineIntel && ((((CpuInfo01.Eax == CPUID_MAKE_INTEL_VERSION_INFO(0, 0b0001, 0, 0b0110, 0b1100, 0) ||
                              CpuInfo01.Eax == CPUID_MAKE_INTEL_VERSION_INFO(0, 0b0010, 0, 0b0110, 0b0110, 0)
                              ) || CpuInfo01.Eax == CPUID_MAKE_INTEL_VERSION_INFO(0, 0b0010, 0, 0b0110, 0b0111, 0)
                             ) || (CpuInfo01.Eax == CPUID_MAKE_INTEL_VERSION_INFO(0, 0b0011, 0, 0b0110, 0b0101, 0) ||
                                   CpuInfo01.Eax == CPUID_MAKE_INTEL_VERSION_INFO(0, 0b0011, 0, 0b0110, 0b0110, 0))
                            ) || CpuInfo01.Eax == CPUID_MAKE_INTEL_VERSION_INFO(0, 0b0011, 0, 0b0110, 0b0111, 0)))
    {
        __favor |= (1 << __FAVOR_ATOM);
    }

    if (CpuIdMaxInputValue < 7)
    {
        CpuInfo7.Ebx = 0;
    } else
    {
        __cpuid(CpuInfo7.Registers, 0);
        if (CpuInfo7.F07_00.FeatureFlags.ERMS)
        {
            __favor |= (1 << __FAVOR_ENFSTRG);
        }
    }
    __isa_available = __ISA_AVAILABLE_SSE2;
    __isa_enabled |= (1 << __ISA_AVAILABLE_SSE2);

    if (!CpuInfo01.F01_00.FeatureInfo.SSE4_2)
    {
        goto _exit;
    }
    __isa_available = __ISA_AVAILABLE_SSE42;
    __isa_enabled |= (1 << __ISA_AVAILABLE_SSE42);

    if (!CpuInfo01.F01_00.FeatureInfo.AVX || !CpuInfo01.F01_00.FeatureInfo.F16C)
    {
        goto _exit;
    }
    XFeatureMask = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
    if (!TEST_FLAGS(XFeatureMask, XSTATE_MASK_LEGACY_SSE | XSTATE_MASK_AVX))
    {
        goto _exit;
    }
    __isa_available = __ISA_AVAILABLE_AVX;
    __isa_enabled |= (1 << __ISA_AVAILABLE_AVX);

    if (!CpuInfo7.F07_00.FeatureFlags.AVX2)
    {
        goto _exit;
    }
    __isa_available = __ISA_AVAILABLE_AVX2;
    __isa_enabled |= (1 << __ISA_AVAILABLE_AVX2);

    /* AVX512F, AVX512DQ, AVX512CD, AVX512BW, AVX512VL */
    if (TEST_FLAGS(CpuInfo7.Ebx, 0xD0030000) && TEST_FLAGS(XFeatureMask, XSTATE_MASK_AVX512))
    {
        __isa_available = __ISA_AVAILABLE_AVX512;
        __isa_enabled |= (1 << __ISA_AVAILABLE_AVX512);
    }

_exit:
    return 0;
}

#endif /*  defined(_M_IX86) */

NTSTATUS CRT_Startup_Init()
{
    __security_init_cookie();

#if defined(_M_IX86)
    _asm { fnclex }
    __isa_available_init();
#endif

    if (_initterm_e(__xi_a, __xi_z) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }
    _initterm(__xc_a, __xc_z);

    return STATUS_SUCCESS;
}

VOID CRT_Startup_Uninit()
{
    _initterm(__xp_a, __xp_z);
    _initterm(__xt_a, __xt_z);
}
