#pragma once

#define CPUID_INTEL_VERSION_INFO_MASK 0xFFF3FF0 // Extended Family ID | Extended Model ID | Processor Type | Family ID | Model
#define CPUID_MAKE_INTEL_VERSION_INFO(ExtendedFamilyId, ExtendedModelId, ProcessorType, FamilyId, Model, SteppingId)\
    (((ExtendedFamilyId & 0b11111111) << 20) |\
     ((ExtendedModelId & 0b1111) << 16) |\
     ((ProcessorType & 0b11) << 12) |\
     ((FamilyId & 0b1111) << 8) |\
     ((Model & 0b1111) << 4) |\
     (SteppingId & 0b1111))

typedef union _CPUID_INFO
{
    int Registers[4];
    struct
    {
        int Eax;
        int Ebx;
        int Ecx;
        int Edx;
    };

    /* F_Leaf_SubLeaf, aka F_EAX_ECX */

    /* 00-03: Basic CPUID Information */
    struct
    {
        /* Eax */
        unsigned int MaxInputValue;
        /* Ebx, Ecx, Edx */
        unsigned int VendorIdString[3];
    } F00_00;

    struct
    {
/*
 * Eax: Intel CPU version information
 *
 *  31 28 27                20 19               16 15 14 13            12 11        8 7     4 3           0
 * +-----+--------------------+-------------------+-----+----------------+-----------+-------+-------------+
 * |     | Extended Family ID | Extended Model ID |     | Processor Type | Family ID | Model | Stepping ID |
 * +-----+--------------------+-------------------+-----+----------------+-----------+-------+-------------+
 *
 */
        struct
        {
            unsigned int SteppingId : 4;
            unsigned int Model : 4;
            unsigned int FamilyId : 4;
            unsigned int ProcessorType : 2;
            unsigned int ReservedBits0 : 2;
            unsigned int ExtendedModelId : 4;
            unsigned int ExtendedFamilyId : 8;
            unsigned int ReservedBits1 : 4;
        } VersionInfo;
        /* Ebx: Additional information */
        struct
        {
            unsigned int BrandIndex : 8;
            unsigned int CLFlushLineSize : 8;
            unsigned int MaxNumberOfAddressableId : 8;
            unsigned int InitialAPICId : 8;
        } AdditionalInfo;
        /* Ecx, Edx: Feature information */
        struct
        {
            /* Ecx */
            struct
            {
                unsigned int SSE3 : 1;                /* 00 SSE3 */
                unsigned int PCLMULQDQ : 1;           /* 01 PCLMULQDQ */
                unsigned int DTES64 : 1;              /* 02 64-bit DS Area */
                unsigned int MONITOR : 1;             /* 03 MONITOR/MWAIT */
                unsigned int DS_CPL : 1;              /* 04 CPL Qualified Debug Store */
                unsigned int VMX : 1;                 /* 05 Virtual Machine Extensions */
                unsigned int SMX : 1;                 /* 06 Safer Mode Extensions */
                unsigned int EIST : 1;                /* 07 Enhanced Intel SpeedStep® technology */
                unsigned int TM2 : 1;                 /* 08 Thermal Monitor 2 */
                unsigned int SSSE3 : 1;               /* 09 SSSE3 */
                unsigned int CNXT_ID : 1;             /* 10 L1 Context ID*/
                unsigned int SDBG : 1;                /* 11 IA32_DEBUG_INTERFACE MSR */
                unsigned int FMA : 1;                 /* 12 FMA */
                unsigned int CMPXCHG16B : 1;          /* 13 CMPXCHG16B Available */
                unsigned int xTPRUpdateControl : 1;   /* 14 xTPR Update Control */
                unsigned int PDCM : 1;                /* 15 Perfmon and Debug Capability */
                unsigned int ReservedBits0 : 1;       /* 16 Reserved */
                unsigned int PCID : 1;                /* 17 Process-context identifiers */
                unsigned int DCA : 1;                 /* 18 Direct Cache Access */
                unsigned int SSE4_1 : 1;              /* 19 SSE4.1*/
                unsigned int SSE4_2 : 1;              /* 20 SSE4.2*/
                unsigned int x2APIC : 1;              /* 21 x2APIC */
                unsigned int MOVBE : 1;               /* 22 MOVBE */
                unsigned int POPCNT : 1;              /* 23 POPCNT */
                unsigned int TSC_Deadline : 1;        /* 24 TSC deadline */
                unsigned int AESNI : 1;               /* 25 AESNI */
                unsigned int XSAVE : 1;               /* 26 XSAVE */
                unsigned int OSXSAVE : 1;             /* 27 CR4.OSXSAVE */
                unsigned int AVX : 1;                 /* 28 AVX */
                unsigned int F16C : 1;                /* 29 16-bit floating-point conversion */
                unsigned int RDRAND : 1;              /* 30 RDRAND */
                unsigned int NotUsedBits0 : 1;        /* 31 Always 0 */
            };
            /* Edx */
            struct
            {
                unsigned int FPU : 1;                 /* 00 x87 FPU */
                unsigned int VME : 1;                 /* 01 Virtual 8086 mode enhancements */
                unsigned int DE : 1;                  /* 02 Debugging Extensions */
                unsigned int PSE : 1;                 /* 03 Page Size Extension */
                unsigned int TSC : 1;                 /* 04 Time Stamp Counter */
                unsigned int MSR : 1;                 /* 05 Model Specific Registers RDMSR and WRMSR Instructions */
                unsigned int PAE : 1;                 /* 06 Physical Address Extension */
                unsigned int MCE : 1;                 /* 07 Machine Check Exception */
                unsigned int CX8 : 1;                 /* 08 CMPXCHG8B Instruction */
                unsigned int APIC : 1;                /* 09 APIC On-Chip */
                unsigned int ReservedBits1 : 1;       /* 10 Reserved */
                unsigned int SEP : 1;                 /* 11 SYSENTER and SYSEXIT Instructions */
                unsigned int MTRR : 1;                /* 12 Memory Type Range Registers */
                unsigned int PGE : 1;                 /* 13 Page Global Bit */
                unsigned int MCA : 1;                 /* 14 Machine Check Architecture */
                unsigned int CMOV : 1;                /* 15 Conditional Move Instructions */
                unsigned int PAT : 1;                 /* 16 Page Attribute Table */
                unsigned int PSE_36 : 1;              /* 17 36-Bit Page Size Extension */
                unsigned int PSN : 1;                 /* 18 Processor Serial Number */
                unsigned int CLFSH : 1;               /* 19 CLFLUSH Instruction */
                unsigned int ReservedBits2 : 1;       /* 20 Reserved */
                unsigned int DS : 1;                  /* 21 Debug Store */
                unsigned int ACPI : 1;                /* 22 Thermal Monitor and Software Controlled Clock Facilities */
                unsigned int MMX : 1;                 /* 23 Intel MMX Technology */
                unsigned int FXSR : 1;                /* 24 FXSAVE and FXRSTOR Instructions */
                unsigned int SSE : 1;                 /* 25 SSE */
                unsigned int SSE2 : 1;                /* 26 SSE2 */
                unsigned int SS : 1;                  /* 27 Self Snoop */
                unsigned int HTT : 1;                 /* 28 Max APIC IDs reserved field is Valid */
                unsigned int TM : 1;                  /* 29 Thermal Monitor */
                unsigned int ReservedBits3 : 1;       /* 30 Reserved */
                unsigned int PBE : 1;                 /* 31 Pending Break Enable */
            };
        } FeatureInfo;
    } F01_00;

    /* 07 Structured Extended Feature Flags Enumeration Leaf */
    struct
    {
        /* Eax */
        unsigned int MaxInputValue;
        /* Ebx, Ecx, Edx: Feature Flags */
        struct
        {
            /* Ebx */
            struct
            {
                unsigned int FSGSBASE : 1;             /* 00 FSGSBASE */
                unsigned int IA32_TSC_ADJUST : 1;      /* 01 IA32_TSC_ADJUST MSR */
                unsigned int SGX : 1;                  /* 02 Intel® Software Guard Extensions */
                unsigned int BMI1 : 1;                 /* 03 BMI1 */
                unsigned int HLE : 1;                  /* 04 HLE */
                unsigned int AVX2 : 1;                 /* 05 AVX2 */
                unsigned int FDP_EXCPTN_ONLY : 1;      /* 06 FDP_EXCPTN_ONLY */
                unsigned int SMEP : 1;                 /* 07 SMEP */
                unsigned int BMI2 : 1;                 /* 08 BMI2 */
                unsigned int ERMS : 1;                 /* 09 Enhanced Fast Strings REP MOVB/STOB */
                unsigned int INVPCID : 1;              /* 10 INVPCID */
                unsigned int RTM : 1;                  /* 11 RTM */
                unsigned int RDT_M : 1;                /* 12 Intel® Resource Director Technology (Intel® RDT) Monitoring */
                unsigned int DeprecatesFPUCSDS : 1;    /* 13 Deprecates FPU CS and FPU DS */
                unsigned int MPX : 1;                  /* 14 Intel® Memory Protection Extensions */
                unsigned int RDT_A : 1;                /* 15 Intel® Resource Director Technology (Intel® RDT) Allocation */
                unsigned int AVX512F : 1;              /* 16 AVX512F */
                unsigned int AVX512DQ : 1;             /* 17 AVX512DQ */
                unsigned int RDSEED : 1;               /* 18 RDSEED */
                unsigned int ADX : 1;                  /* 19 ADX */
                unsigned int SMAP : 1;                 /* 20 Supervisor-Mode Access Prevention */
                unsigned int AVX512_IFMA : 1;          /* 21 AVX512_IFMA */
                unsigned int ReservedBits0 : 1;        /* 22 Reserved */
                unsigned int CLFLUSHOPT : 1;           /* 23 CLFLUSHOPT */
                unsigned int CLWB : 1;                 /* 24 CLWB */
                unsigned int PT : 1;                   /* 25 Intel Processor Trace */
                unsigned int AVX512PF : 1;             /* 26 AVX512PF */
                unsigned int AVX512ER : 1;             /* 27 AVX512ER */
                unsigned int AVX512CD : 1;             /* 28 AVX512CD */
                unsigned int SHA : 1;                  /* 29 SHA */
                unsigned int AVX512BW : 1;             /* 30 AVX512BW */
                unsigned int AVX512VL : 1;             /* 31 AVX512VL */
            };
            /* Ecx */
            struct
            {
                unsigned int PREFETCHWT1 : 1;          /* 00 PREFETCHWT1 */
                unsigned int AVX512_VBMI : 1;          /* 01 AVX512_VBMI */
                unsigned int UMIP : 1;                 /* 02 User-mode instruction prevention */
                unsigned int PKU : 1;                  /* 03 Protection keys for user-mode pages */
                unsigned int OSPKE : 1;                /* 04 CR4.PKE */
                unsigned int WAITPKG : 1;              /* 05 WAITPKG */
                unsigned int AVX512_VBMI2 : 1;         /* 06 AVX512_VBMI2 */
                unsigned int CET_SS : 1;               /* 07 CET shadow stack */
                unsigned int GFNI : 1;                 /* 08 GFNI */
                unsigned int VAES : 1;                 /* 09 VAES */
                unsigned int VPCLMULQDQ : 1;           /* 10 VPCLMULQDQ */
                unsigned int AVX512_VNNI : 1;          /* 11 AVX512_VNNI */
                unsigned int AVX512_BITALG : 1;        /* 12 AVX512_BITALG */
                unsigned int TME_EN : 1;               /* 13 TME_EN */
                unsigned int AVX512_VPOPCNTDQ : 1;     /* 14 AVX512_VPOPCNTDQ */
                unsigned int ReservedBits1 : 1;        /* 15 Reserved */
                unsigned int LA57 : 1;                 /* 16 57-bit linear addresses and five-level paging */
                unsigned int MAWAU : 5;                /* 17-21 57-bit linear addresses and five-level paging */
                unsigned int RDPID : 1;                /* 22 RDPID and IA32_TSC_AUX */
                unsigned int KL : 1;                   /* 23 Key Locker */
                unsigned int ReservedBits2 : 1;        /* 24 Reserved */
                unsigned int CLDEMOTE : 1;             /* 25 Cache line demote */
                unsigned int ReservedBits3 : 1;        /* 26 Reserved */
                unsigned int MOVDIRI : 1;              /* 27 MOVDIRI */
                unsigned int MOVDIR64B : 1;            /* 28 MOVDIR64B */
                unsigned int ENQCMD : 1;               /* 29 Enqueue Stores */
                unsigned int SGX_LC : 1;               /* 30 SGX Launch Configuration */
                unsigned int PKS : 1;                  /* 31 Protection keys for supervisor-mode pages */
            };
            /* Edx */
            struct
            {
                unsigned int ReservedBits4 : 1;            /* 00 Reserved */
                unsigned int SGX_KEYS : 1;                 /* 01 Attestation Services for Intel® SGX */
                unsigned int AVX512_4VNNIW : 1;            /* 02 AVX512_4VNNIW */
                unsigned int AVX512_4FMAPS : 1;            /* 03 AVX512_4FMAPS */
                unsigned int FSRM : 1;                     /* 04 Fast Short REP MOV */
                unsigned int UINTR : 1;                    /* 05 User interrupts */
                unsigned int ReservedBits5 : 2;            /* 06-07 Reserved */
                unsigned int AVX512_VP2INTERSECT : 1;      /* 08 AVX512_VP2INTERSECT */
                unsigned int SRBDS_CTRL : 1;               /* 09 SRBDS_CTRL */
                unsigned int MD_CLEAR : 1;                 /* 10 MD_CLEAR */
                unsigned int RTM_ALWAYS_ABORT : 1;         /* 11 RTM_ALWAYS_ABORT */
                unsigned int ReservedBits6 : 1;            /* 12 Reserved */
                unsigned int RTM_FORCE_ABORT : 1;          /* 13 RTM_FORCE_ABORT */
                unsigned int SERIALIZE : 1;                /* 14 SERIALIZE */
                unsigned int Hybrid : 1;                   /* 15 Hybrid */
                unsigned int TSXLDTRK : 1;                 /* 16 Intel TSX suspend/resume of load address tracking */
                unsigned int ReservedBits7 : 1;            /* 17 Reserved */
                unsigned int PCONFIG : 1;                  /* 18 PCONFIG */
                unsigned int ArchitecturalLBRs : 1;        /* 19 Architectural LBRs */
                unsigned int CET_IBT : 1;                  /* 20 CET indirect branch tracking */
                unsigned int ReservedBits8 : 1;            /* 21 Reserved */
                unsigned int AMX_BF16 : 1;                 /* 22 Tile computational operations on bfloat16 numbers */
                unsigned int AVX512_FP16 : 1;              /* 23 AVX512_FP16 */
                unsigned int AMX_TILE : 1;                 /* 24 Tile architecture */
                unsigned int AMX_INT8 : 1;                 /* 25 Tile computational operations on 8-bit integers */
                unsigned int Enum_IBRS_IBPB : 1;           /* 26 Enumerates for IBRS and IBPB */
                unsigned int Enum_STIBP : 1;               /* 27 Enumerates for STIBP */
                unsigned int Enum_L1D_FLUSH : 1;           /* 28 Enumerates for L1D_FLUSH */
                unsigned int Enum_ARCH_CAPABILITIES : 1;   /* 29 Enumerates for the IA32_ARCH_CAPABILITIES MSR */
                unsigned int Enum_CORE_CAPABILITIES : 1;   /* 30 Enumerates for the IA32_CORE_CAPABILITIES MSR */
                unsigned int Enum_SSBD : 1;                /* 31 Enumerates for SSBD */
            };
        } FeatureFlags;
    } F07_00;

    union
    {
        /* 0 if the sub-leaf index, 1, is invalid */
        struct
        {
            unsigned int EaxValidity;
            unsigned int EbxValidity;
            unsigned int EcxValidity;
            unsigned int EdxValidity;
        };
        /* Eax, Ebx, Ecx, Edx: Feature Flags */
        struct
        {
            /* Eax */
            struct
            {
                unsigned int ReservedBits0 : 4;    /* 00-03 Reserved */
                unsigned int AVX_VNNI : 1;         /* 04 AVX-VNNI */
                unsigned int AVX512_BF16 : 1;      /* 05 AVX512_BF16 */
                unsigned int ReservedBits1 : 4;    /* 06-09 Reserved */
                unsigned int FZRM : 1;             /* 10 Fast zero-length REP MOVSB */
                unsigned int FSRS : 1;             /* 11 Fast short REP STOSB */
                unsigned int FSRCS : 1;            /* 12 Fast short REP CMPSB/SCASB */
                unsigned int ReservedBits2 : 9;    /* 13-21 Reserved */
                unsigned int HRESET : 1;           /* 22 HRESET */
                unsigned int ReservedBits3 : 9;    /* 23-31 Reserved */
            };
            /* Ebx */
            struct
            {
                unsigned int Enum_PPIN : 1;        /* 00 Enumerates the presence of the IA32_PPIN and IA32_PPIN_CTL MSRs */
                unsigned int ReservedBits4 : 31;   /* 01-31 Reserved */
            };
            /* Ecx */
            unsigned int ReservedBits5;
            /* Edx */
            struct
            {
                unsigned int ReservedBits6 : 18;   /* 00-17 Reserved */
                unsigned int CET_SSS : 1;          /* 18 CET_SSS */
                unsigned int ReservedBits7 : 13;   /* 19-31 Reserved */
            };
        };
    } F07_01;

} CPUID_INFO, *PCPUID_INFO;
