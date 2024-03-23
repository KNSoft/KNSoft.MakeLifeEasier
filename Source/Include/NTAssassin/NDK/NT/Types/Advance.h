#pragma once

#include "../MinDef.h"

#include "Rtl.h"

#pragma region PEB_LDR_DATA[64/32]

typedef struct _PEB_LDR_DATA
{
    ULONG Length;
    BOOL Initialized;
    PVOID SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
    PVOID EntryInProgress;
    PVOID ShutdownInProgress;
    PVOID ShutdownThreadId;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

#if defined(_NTASSASSIN_NDK_EXTENSION_)

typedef struct _PEB_LDR_DATA64
{
    ULONG Length;
    BOOL Initialized;
    VOID* POINTER_64 SsHandle;
    LIST_ENTRY64 InLoadOrderModuleList;
    LIST_ENTRY64 InMemoryOrderModuleList;
    LIST_ENTRY64 InInitializationOrderModuleList;
    VOID* POINTER_64 EntryInProgress;
    VOID* POINTER_64 ShutdownInProgress;
    VOID* POINTER_64 ShutdownThreadId;
} PEB_LDR_DATA64, *PPEB_LDR_DATA64;

typedef struct _PEB_LDR_DATA32
{
    ULONG Length;
    BOOL Initialized;
    VOID* POINTER_32 SsHandle;
    LIST_ENTRY32 InLoadOrderModuleList;
    LIST_ENTRY32 InMemoryOrderModuleList;
    LIST_ENTRY32 InInitializationOrderModuleList;
    VOID* POINTER_32 EntryInProgress;
    VOID* POINTER_32 ShutdownInProgress;
    VOID* POINTER_32 ShutdownThreadId;
} PEB_LDR_DATA32, *PPEB_LDR_DATA32;

#endif /* defined(_NTASSASSIN_NDK_EXTENSION_) */

#pragma endregion

#pragma region PEB[64/32]

typedef struct _PEB
{
    UCHAR InheritedAddressSpace;
    UCHAR ReadImageFileExecOptions;
    UCHAR BeingDebugged;
    union
    {
        UCHAR BitField;
        struct
        {
            UCHAR ImageUsesLargePages : 1;
            UCHAR IsProtectedProcess : 1;
            UCHAR IsImageDynamicallyRelocated : 1;
            UCHAR SkipPatchingUser32Forwarders : 1;
            UCHAR IsPackagedProcess : 1;
            UCHAR IsAppContainer : 1;
            UCHAR IsProtectedProcessLight : 1;
            UCHAR IsLongPathAwareProcess : 1;
        };
    };
#if _WIN64
    UCHAR Padding0[4];
#endif
    HANDLE Mutant;
    PVOID ImageBaseAddress;
    PPEB_LDR_DATA Ldr;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    PVOID SubSystemData;
    HANDLE ProcessHeap;
    PRTL_CRITICAL_SECTION FastPebLock;
    PSLIST_HEADER AtlThunkSListPtr;
    PVOID IFEOKey;
    union
    {
        ULONG CrossProcessFlags;
        struct
        {
            ULONG ProcessInJob : 1;
            ULONG ProcessInitializing : 1;
            ULONG ProcessUsingVEH : 1;
            ULONG ProcessUsingVCH : 1;
            ULONG ProcessUsingFTH : 1;
            ULONG ProcessPreviouslyThrottled : 1;
            ULONG ProcessCurrentlyThrottled : 1;
            ULONG ProcessImagesHotPatched : 1;
            ULONG ReservedBits0 : 24;
        };
    };
#if _WIN64
    UCHAR Padding1[4];
#endif
    union
    {
        PVOID KernelCallbackTable;
        PVOID UserSharedInfoPtr;
    };
    ULONG SystemReserved;
    ULONG AtlThunkSListPtr32;
    PVOID ApiSetMap;
    ULONG TlsExpansionCounter;
#if _WIN64
    UCHAR Padding2[4];
#endif
    PRTL_BITMAP TlsBitmap;
    ULONG TlsBitmapBits[2];
    PVOID ReadOnlySharedMemoryBase;
    PVOID SharedData;
    PVOID* ReadOnlyStaticServerData;
    PVOID AnsiCodePageData;
    PVOID OemCodePageData;
    PVOID UnicodeCaseTableData;
    ULONG NumberOfProcessors;
    ULONG NtGlobalFlag;
    LARGE_INTEGER CriticalSectionTimeout;
    ULONG_PTR HeapSegmentReserve;
    ULONG_PTR HeapSegmentCommit;
    ULONG_PTR HeapDeCommitTotalFreeThreshold;
    ULONG_PTR HeapDeCommitFreeBlockThreshold;
    ULONG NumberOfHeaps;
    ULONG MaximumNumberOfHeaps;
    PVOID ProcessHeaps;
    PVOID GdiSharedHandleTable;
    PVOID ProcessStarterHelper;
    ULONG GdiDCAttributeList;
#if _WIN64
    UCHAR Padding3[4];
#endif
    PRTL_CRITICAL_SECTION LoaderLock;
    ULONG OSMajorVersion;
    ULONG OSMinorVersion;
    USHORT OSBuildNumber;
    USHORT OSCSDVersion;
    ULONG OSPlatformId;
    ULONG ImageSubsystem;
    ULONG ImageSubsystemMajorVersion;
    ULONG ImageSubsystemMinorVersion;
#if _WIN64
    UCHAR Padding4[4];
#endif
    ULONG_PTR ActiveProcessAffinityMask;
#if _WIN64
    ULONG GdiHandleBuffer[60];
#else
    ULONG GdiHandleBuffer[34];
#endif
    PVOID PostProcessInitRoutine;
    PRTL_BITMAP TlsExpansionBitmap;
    ULONG TlsExpansionBitmapBits[32];
    ULONG SessionId;
#ifdef _WIN64
    UCHAR Padding5[4];
#endif
    ULARGE_INTEGER AppCompatFlags;
    ULARGE_INTEGER AppCompatFlagsUser;
    PVOID pShimData;
    PVOID AppCompatInfo;
    UNICODE_STRING CSDVersion;
    PACTIVATION_CONTEXT_DATA ActivationContextData;
    PASSEMBLY_STORAGE_MAP ProcessAssemblyStorageMap;
    PACTIVATION_CONTEXT_DATA SystemDefaultActivationContextData;
    PASSEMBLY_STORAGE_MAP SystemAssemblyStorageMap;
    ULONG_PTR MinimumStackCommit;
    PVOID SparePointers[2];
    PVOID PatchLoaderData;
    struct CHPEV2_PROCESS_INFO* ChpeV2ProcessInfo;
    ULONG AppModelFeatureState;
    ULONG SpareUlongs[2];
    USHORT ActiveCodePage;
    USHORT OemCodePage;
    USHORT UseCaseMapping;
    USHORT UnusedNlsField;
    PVOID WerRegistrationData;
    PVOID WerShipAssertPtr;
#ifdef _WIN64
    PVOID EcCodeBitMap;
#else
    PVOID Spare;
#endif
    PVOID pImageHeaderHash;
    union
    {
        ULONG TracingFlags;
        struct
        {
            ULONG HeapTracingEnabled : 1;
            ULONG CritSecTracingEnabled : 1;
            ULONG LibLoaderTracingEnabled : 1;
            ULONG SpareTracingBits : 29;
        };
    };
#ifdef _WIN64
    UCHAR Padding6[4];
#endif
    ULONGLONG CsrServerReadOnlySharedMemoryBase;
    ULONG_PTR TppWorkerpListLock;
    LIST_ENTRY TppWorkerpList;
    PVOID WaitOnAddressHashTable[128];
    PVOID TelemetryCoverageHeader;
    ULONG CloudFileFlags;
    ULONG CloudFileDiagFlags;
    CHAR PlaceholderCompatibilityMode;
    CHAR PlaceholderCompatibilityModeReserved[7];
    PLEAP_SECOND_DATA LeapSecondData;
    union
    {
        ULONG LeapSecondFlags;
        struct
        {
            ULONG SixtySecondEnabled : 1;
            ULONG Reserved : 31;
        };
    };
    ULONG NtGlobalFlag2;
    ULONGLONG ExtendedFeatureDisableMask;
} PEB, *PPEB;

#if defined(_NTASSASSIN_NDK_EXTENSION_)

typedef struct _PEB64
{
    UCHAR InheritedAddressSpace;
    UCHAR ReadImageFileExecOptions;
    UCHAR BeingDebugged;
    union
    {
        UCHAR BitField;
        struct
        {
            UCHAR ImageUsesLargePages : 1;
            UCHAR IsProtectedProcess : 1;
            UCHAR IsImageDynamicallyRelocated : 1;
            UCHAR SkipPatchingUser32Forwarders : 1;
            UCHAR IsPackagedProcess : 1;
            UCHAR IsAppContainer : 1;
            UCHAR IsProtectedProcessLight : 1;
            UCHAR IsLongPathAwareProcess : 1;
        };
    };
#if _WIN64
    UCHAR Padding0[4];
#endif
    VOID* POINTER_64 Mutant;
    VOID* POINTER_64 ImageBaseAddress;
    PEB_LDR_DATA64* POINTER_64 Ldr;
    PRTL_USER_PROCESS_PARAMETERS64 ProcessParameters;
    VOID* POINTER_64 SubSystemData;
    VOID* POINTER_64 ProcessHeap;
    RTL_CRITICAL_SECTION64* POINTER_64 FastPebLock;
    struct SLIST_HEADER* POINTER_64 AtlThunkSListPtr; // FIXME: SLIST_HEADER is depends on platform
    VOID* POINTER_64 IFEOKey;
    union
    {
        ULONG CrossProcessFlags;
        struct
        {
            ULONG ProcessInJob : 1;
            ULONG ProcessInitializing : 1;
            ULONG ProcessUsingVEH : 1;
            ULONG ProcessUsingVCH : 1;
            ULONG ProcessUsingFTH : 1;
            ULONG ProcessPreviouslyThrottled : 1;
            ULONG ProcessCurrentlyThrottled : 1;
            ULONG ProcessImagesHotPatched : 1;
            ULONG ReservedBits0 : 24;
        };
    };
    UCHAR Padding1[4];
    union
    {
        VOID* POINTER_64 KernelCallbackTable;
        VOID* POINTER_64 UserSharedInfoPtr;
    };
    ULONG SystemReserved;
    ULONG AtlThunkSListPtr32;
    VOID* POINTER_64 ApiSetMap;
    ULONG TlsExpansionCounter;
    UCHAR Padding2[4];
    RTL_BITMAP64* POINTER_64 TlsBitmap;
    ULONG TlsBitmapBits[2];
    VOID* POINTER_64 ReadOnlySharedMemoryBase;
    VOID* POINTER_64 SharedData;
    VOID* POINTER_64* POINTER_64 ReadOnlyStaticServerData;
    VOID* POINTER_64 AnsiCodePageData;
    VOID* POINTER_64 OemCodePageData;
    VOID* POINTER_64 UnicodeCaseTableData;
    ULONG NumberOfProcessors;
    ULONG NtGlobalFlag;
    LARGE_INTEGER CriticalSectionTimeout;
    ULONGLONG HeapSegmentReserve;
    ULONGLONG HeapSegmentCommit;
    ULONGLONG HeapDeCommitTotalFreeThreshold;
    ULONGLONG HeapDeCommitFreeBlockThreshold;
    ULONG NumberOfHeaps;
    ULONG MaximumNumberOfHeaps;
    VOID* POINTER_64 ProcessHeaps;
    VOID* POINTER_64 GdiSharedHandleTable;
    VOID* POINTER_64 ProcessStarterHelper;
    ULONG GdiDCAttributeList;
    UCHAR Padding3[4];
    RTL_CRITICAL_SECTION64* POINTER_64 LoaderLock;
    ULONG OSMajorVersion;
    ULONG OSMinorVersion;
    USHORT OSBuildNumber;
    USHORT OSCSDVersion;
    ULONG OSPlatformId;
    ULONG ImageSubsystem;
    ULONG ImageSubsystemMajorVersion;
    ULONG ImageSubsystemMinorVersion;
    UCHAR Padding4[4];
    ULONGLONG ActiveProcessAffinityMask;
    ULONG GdiHandleBuffer[60];
    VOID* POINTER_64 PostProcessInitRoutine;
    RTL_BITMAP64* POINTER_64 TlsExpansionBitmap;
    ULONG TlsExpansionBitmapBits[32];
    ULONG SessionId;
    UCHAR Padding5[4];
    ULARGE_INTEGER AppCompatFlags;
    ULARGE_INTEGER AppCompatFlagsUser;
    VOID* POINTER_64 pShimData;
    VOID* POINTER_64 AppCompatInfo;
    UNICODE_STRING64 CSDVersion;
    ACTIVATION_CONTEXT_DATA* POINTER_64 ActivationContextData;
    ASSEMBLY_STORAGE_MAP* POINTER_64 ProcessAssemblyStorageMap;
    ACTIVATION_CONTEXT_DATA* POINTER_64 SystemDefaultActivationContextData;
    ASSEMBLY_STORAGE_MAP* POINTER_64 SystemAssemblyStorageMap;
    ULONGLONG MinimumStackCommit;
    VOID* POINTER_64 SparePointers[2];
    VOID* POINTER_64 PatchLoaderData;
    struct CHPEV2_PROCESS_INFO* POINTER_64 ChpeV2ProcessInfo;
    ULONG AppModelFeatureState;
    ULONG SpareUlongs[2];
    USHORT ActiveCodePage;
    USHORT OemCodePage;
    USHORT UseCaseMapping;
    USHORT UnusedNlsField;
    VOID* POINTER_64 WerRegistrationData;
    VOID* POINTER_64 WerShipAssertPtr;
    VOID* POINTER_64 EcCodeBitMap;
    VOID* POINTER_64 pImageHeaderHash;
    union
    {
        ULONG TracingFlags;
        struct
        {
            ULONG HeapTracingEnabled : 1;
            ULONG CritSecTracingEnabled : 1;
            ULONG LibLoaderTracingEnabled : 1;
            ULONG SpareTracingBits : 29;
        };
    };
    UCHAR Padding6[4];
    ULONGLONG CsrServerReadOnlySharedMemoryBase;
    ULONGLONG TppWorkerpListLock;
    LIST_ENTRY64 TppWorkerpList;
    VOID* POINTER_64 WaitOnAddressHashTable[128];
    VOID* POINTER_64 TelemetryCoverageHeader;
    ULONG CloudFileFlags;
    ULONG CloudFileDiagFlags;
    CHAR PlaceholderCompatibilityMode;
    CHAR PlaceholderCompatibilityModeReserved[7];
    LEAP_SECOND_DATA* POINTER_64 LeapSecondData;
    union
    {
        ULONG LeapSecondFlags;
        struct
        {
            ULONG SixtySecondEnabled : 1;
            ULONG Reserved : 31;
        };
    };
    ULONG NtGlobalFlag2;
    ULONGLONG ExtendedFeatureDisableMask;
} PEB64, *PPEB64;

typedef struct _PEB32
{
    UCHAR InheritedAddressSpace;
    UCHAR ReadImageFileExecOptions;
    UCHAR BeingDebugged;
    union
    {
        UCHAR BitField;
        struct
        {
            UCHAR ImageUsesLargePages : 1;
            UCHAR IsProtectedProcess : 1;
            UCHAR IsImageDynamicallyRelocated : 1;
            UCHAR SkipPatchingUser32Forwarders : 1;
            UCHAR IsPackagedProcess : 1;
            UCHAR IsAppContainer : 1;
            UCHAR IsProtectedProcessLight : 1;
            UCHAR IsLongPathAwareProcess : 1;
        };
    };
    VOID* POINTER_32 Mutant;
    VOID* POINTER_32 ImageBaseAddress;
    PEB_LDR_DATA32* POINTER_32 Ldr;
    RTL_USER_PROCESS_PARAMETERS32* POINTER_32 ProcessParameters;
    VOID* POINTER_32 SubSystemData;
    VOID* POINTER_32 ProcessHeap;
    RTL_CRITICAL_SECTION32* POINTER_32 FastPebLock;
    struct SLIST_HEADER* POINTER_32 AtlThunkSListPtr; // FIXME: SLIST_HEADER is depends on platform
    VOID* POINTER_32 IFEOKey;
    union
    {
        ULONG CrossProcessFlags;
        struct
        {
            ULONG ProcessInJob : 1;
            ULONG ProcessInitializing : 1;
            ULONG ProcessUsingVEH : 1;
            ULONG ProcessUsingVCH : 1;
            ULONG ProcessUsingFTH : 1;
            ULONG ProcessPreviouslyThrottled : 1;
            ULONG ProcessCurrentlyThrottled : 1;
            ULONG ProcessImagesHotPatched : 1;
            ULONG ReservedBits0 : 24;
        };
    };
    union
    {
        VOID* POINTER_32 KernelCallbackTable;
        VOID* POINTER_32 UserSharedInfoPtr;
    };
    ULONG SystemReserved;
    ULONG AtlThunkSListPtr32;
    VOID* POINTER_32 ApiSetMap;
    ULONG TlsExpansionCounter;
    RTL_BITMAP32* POINTER_32 TlsBitmap;
    ULONG TlsBitmapBits[2];
    VOID* POINTER_32 ReadOnlySharedMemoryBase;
    VOID* POINTER_32 SharedData;
    VOID* POINTER_32* POINTER_32 ReadOnlyStaticServerData;
    VOID* POINTER_32 AnsiCodePageData;
    VOID* POINTER_32 OemCodePageData;
    VOID* POINTER_32 UnicodeCaseTableData;
    ULONG NumberOfProcessors;
    ULONG NtGlobalFlag;
    LARGE_INTEGER CriticalSectionTimeout;
    ULONG HeapSegmentReserve;
    ULONG HeapSegmentCommit;
    ULONG HeapDeCommitTotalFreeThreshold;
    ULONG HeapDeCommitFreeBlockThreshold;
    ULONG NumberOfHeaps;
    ULONG MaximumNumberOfHeaps;
    VOID* POINTER_32 ProcessHeaps;
    VOID* POINTER_32 GdiSharedHandleTable;
    VOID* POINTER_32 ProcessStarterHelper;
    ULONG GdiDCAttributeList;
    RTL_CRITICAL_SECTION32* POINTER_32 LoaderLock;
    ULONG OSMajorVersion;
    ULONG OSMinorVersion;
    USHORT OSBuildNumber;
    USHORT OSCSDVersion;
    ULONG OSPlatformId;
    ULONG ImageSubsystem;
    ULONG ImageSubsystemMajorVersion;
    ULONG ImageSubsystemMinorVersion;
    ULONG ActiveProcessAffinityMask;
    ULONG GdiHandleBuffer[34];
    VOID* POINTER_32 PostProcessInitRoutine;
    RTL_BITMAP32* POINTER_32 TlsExpansionBitmap;
    ULONG TlsExpansionBitmapBits[32];
    ULONG SessionId;
    ULARGE_INTEGER AppCompatFlags;
    ULARGE_INTEGER AppCompatFlagsUser;
    VOID* POINTER_32 pShimData;
    VOID* POINTER_32 AppCompatInfo;
    UNICODE_STRING32 CSDVersion;
    ACTIVATION_CONTEXT_DATA* POINTER_32 ActivationContextData;
    ASSEMBLY_STORAGE_MAP32* POINTER_32 ProcessAssemblyStorageMap;
    ACTIVATION_CONTEXT_DATA* POINTER_32 SystemDefaultActivationContextData;
    ASSEMBLY_STORAGE_MAP32* POINTER_32 SystemAssemblyStorageMap;
    ULONG MinimumStackCommit;
    VOID* POINTER_32 SparePointers[2];
    VOID* POINTER_32 PatchLoaderData;
    struct CHPEV2_PROCESS_INFO* POINTER_32 ChpeV2ProcessInfo;
    ULONG AppModelFeatureState;
    ULONG SpareUlongs[2];
    USHORT ActiveCodePage;
    USHORT OemCodePage;
    USHORT UseCaseMapping;
    USHORT UnusedNlsField;
    VOID* POINTER_32 WerRegistrationData;
    VOID* POINTER_32 WerShipAssertPtr;
    VOID* POINTER_32 Spare;
    VOID* POINTER_32 pImageHeaderHash;
    union
    {
        ULONG TracingFlags;
        struct
        {
            ULONG HeapTracingEnabled : 1;
            ULONG CritSecTracingEnabled : 1;
            ULONG LibLoaderTracingEnabled : 1;
            ULONG SpareTracingBits : 29;
        };
    };
    ULONGLONG CsrServerReadOnlySharedMemoryBase;
    ULONG TppWorkerpListLock;
    LIST_ENTRY32 TppWorkerpList;
    VOID* POINTER_32 WaitOnAddressHashTable[128];
    VOID* POINTER_32 TelemetryCoverageHeader;
    ULONG CloudFileFlags;
    ULONG CloudFileDiagFlags;
    CHAR PlaceholderCompatibilityMode;
    CHAR PlaceholderCompatibilityModeReserved[7];
    LEAP_SECOND_DATA* POINTER_32 LeapSecondData;
    union
    {
        ULONG LeapSecondFlags;
        struct
        {
            ULONG SixtySecondEnabled : 1;
            ULONG Reserved : 31;
        };
    };
    ULONG NtGlobalFlag2;
    ULONGLONG ExtendedFeatureDisableMask;
} PEB32, *PPEB32;

#endif /* defined(_NTASSASSIN_NDK_EXTENSION_) */

#pragma endregion

#pragma region TEB_ACTIVE_FRAME[_CONTEXT][64/32]

typedef struct _TEB_ACTIVE_FRAME_CONTEXT
{
    ULONG Flags;
    UCHAR Padding[4];
    PCHAR FrameName;
} TEB_ACTIVE_FRAME_CONTEXT, *PTEB_ACTIVE_FRAME_CONTEXT;

typedef struct _TEB_ACTIVE_FRAME TEB_ACTIVE_FRAME, *PTEB_ACTIVE_FRAME;

struct _TEB_ACTIVE_FRAME
{
    DWORD Flags;
#if _WIN64
    UCHAR Padding[4];
#endif
    struct TEB_ACTIVE_FRAME* Previous;
    PTEB_ACTIVE_FRAME_CONTEXT Context;
};

#if defined(_NTASSASSIN_NDK_EXTENSION_)

typedef struct _TEB_ACTIVE_FRAME_CONTEXT64
{
    ULONG Flags;
    UCHAR Padding[4];
    CHAR* POINTER_64 FrameName;
} TEB_ACTIVE_FRAME_CONTEXT64, *PTEB_ACTIVE_FRAME_CONTEXT64;

typedef struct _TEB_ACTIVE_FRAME_CONTEXT32
{
    ULONG Flags;
    UCHAR Padding[4];
    CHAR* POINTER_32 FrameName;
} TEB_ACTIVE_FRAME_CONTEXT32, *PTEB_ACTIVE_FRAME_CONTEXT32;

typedef struct _TEB_ACTIVE_FRAME64 TEB_ACTIVE_FRAME64, *PTEB_ACTIVE_FRAME64;
typedef struct _TEB_ACTIVE_FRAME32 TEB_ACTIVE_FRAME32, *PTEB_ACTIVE_FRAME32;

struct _TEB_ACTIVE_FRAME64
{
    DWORD Flags;
    UCHAR Padding[4];
    struct TEB_ACTIVE_FRAME64* Previous;
    TEB_ACTIVE_FRAME_CONTEXT64* POINTER_64 Context;
};

struct _TEB_ACTIVE_FRAME32
{
    DWORD Flags;
    struct TEB_ACTIVE_FRAME32* Previous;
    TEB_ACTIVE_FRAME_CONTEXT32* POINTER_32 Context;
};

#endif /* defined(_NTASSASSIN_NDK_EXTENSION_) */

#pragma endregion

#pragma region TEB[64/32]

typedef struct _TEB
{
    NT_TIB NtTib;
    LPWSTR EnvironmentPointer;
    CLIENT_ID ClientId;
    PVOID ActiveRpcHandle;
    PVOID ThreadLocalStoragePointer;
    PPEB ProcessEnvironmentBlock;
    ULONG LastErrorValue;
    ULONG CountOfOwnedCriticalSections;
    PVOID CsrClientThread;
    PVOID Win32ThreadInfo;
    ULONG User32Reserved[26];
    ULONG UserReserved[5];
    PVOID WOW32Reserved;
    ULONG CurrentLocale;
    ULONG FpSoftwareStatusRegister;
    PVOID ReservedForDebuggerInstrumentation[16];
#ifdef _WIN64
    PVOID SystemReserved1[30];
#else
    PVOID SystemReserved1[26];
#endif
    CHAR PlaceholderCompatibilityMode;
    UCHAR PlaceholderHydrationAlwaysExplicit;
    CHAR PlaceholderReserved[10];
    ULONG ProxiedProcessId;
    ACTIVATION_CONTEXT_STACK _ActivationStack;
    UCHAR WorkingOnBehalfTicket[8];
    LONG ExceptionCode;
#ifdef _WIN64
    UCHAR Padding0[4];
#endif
    PACTIVATION_CONTEXT_STACK ActivationContextStackPointer;
    PVOID InstrumentationCallbackSp;
    PVOID InstrumentationCallbackPreviousPc;
    PVOID InstrumentationCallbackPreviousSp;
#ifdef _WIN64
    ULONG TxFsContext;
    BOOLEAN InstrumentationCallbackDisabled;
    UCHAR UnalignedLoadStoreExceptions;
    UCHAR Padding1[2];
#else
    UCHAR InstrumentationCallbackDisabled;
    UCHAR SpareBytes[23];
    DWORD TxFsContext;
#endif
    GDI_TEB_BATCH GdiTebBatch;
    CLIENT_ID RealClientId;
    PVOID GdiCachedProcessHandle;
    ULONG GdiClientPID;
    ULONG GdiClientTID;
    PVOID GdiThreadLocalInfo;
    ULONG_PTR Win32ClientInfo[62];
    PVOID glDispatchTable[233];
    ULONG_PTR glReserved1[29];
    PVOID glReserved2;
    PVOID glSectionInfo;
    PVOID glSection;
    PVOID glTable;
    PVOID glCurrentRC;
    PVOID glContext;
    ULONG LastStatusValue;
#ifdef _WIN64
    UCHAR Padding2[4];
#endif
    UNICODE_STRING StaticUnicodeString;
    WCHAR StaticUnicodeBuffer[261];
#ifdef _WIN64
    UCHAR Padding3[6];
#endif
    PVOID DeallocationStack;
    PVOID TlsSlots[64];
    LIST_ENTRY TlsLinks;
    PVOID Vdm;
    PVOID ReservedForNtRpc;
    PVOID DbgSsReserved[2];
    ULONG HardErrorMode;
#ifdef _WIN64
    UCHAR Padding4[4];
    PVOID Instrumentation[11];
#else
    PVOID Instrumentation[9];
#endif
    GUID ActivityId;
    PVOID SubProcessTag;
    PVOID PerflibData;
    PVOID EtwTraceData;
    PVOID WinSockData;
    ULONG GdiBatchCount;
    union
    {
        PROCESSOR_NUMBER CurrentIdealProcessor;
        union
        {
            DWORD IdealProcessorValue;
            struct
            {
                UCHAR ReservedPad0;
                UCHAR ReservedPad1;
                UCHAR ReservedPad2;
                UCHAR IdealProcessor;
            };
        };
    };
    ULONG GuaranteedStackBytes;
#ifdef _WIN64
    UCHAR Padding5[4];
#endif
    PVOID ReservedForPerf;
    PVOID ReservedForOle;
    ULONG WaitingOnLoaderLock;
#ifdef _WIN64
    UCHAR Padding6[4];
#endif
    PVOID SavedPriorityState;
    ULONG_PTR ReservedForCodeCoverage;
    PVOID ThreadPoolData;
    PVOID TlsExpansionSlots;
#ifdef _WIN64
    struct CHPEV2_CPUAREA_INFO* POINTER_64 ChpeV2CpuAreaInfo;
    PVOID Unused;
#endif
    ULONG MuiGeneration;
    ULONG IsImpersonating;
    PVOID NlsCache;
    PVOID pShimData;
    ULONG HeapData;
#ifdef _WIN64
    UCHAR Padding7[4];
#endif
    PVOID CurrentTransactionHandle;
    PTEB_ACTIVE_FRAME ActiveFrame;
    PVOID FlsData;
    PVOID PreferredLanguages;
    PVOID UserPrefLanguages;
    PVOID MergedPrefLanguages;
    ULONG MuiImpersonation;
    union
    {
        USHORT CrossTebFlags;
        struct
        {
            USHORT SpareCrossTebBits : 16;
        };
    };
    union
    {
        USHORT SameTebFlags;
        struct
        {
            USHORT SafeThunkCall : 1;
            USHORT InDebugPrint : 1;
            USHORT HasFiberData : 1;
            USHORT SkipThreadAttach : 1;
            USHORT WerInShipAssertCode : 1;
            USHORT RanProcessInit : 1;
            USHORT ClonedThread : 1;
            USHORT SuppressDebugMsg : 1;
            USHORT DisableUserStackWalk : 1;
            USHORT RtlExceptionAttached : 1;
            USHORT InitialThread : 1;
            USHORT SessionAware : 1;
            USHORT LoadOwner : 1;
            USHORT LoaderWorker : 1;
            USHORT SkipLoaderInit : 1;
            USHORT SkipFileAPIBrokering : 1;
        };
    };
    PVOID TxnScopeEnterCallback;
    PVOID TxnScopeExitCallback;
    PVOID TxnScopeContext;
    ULONG LockCount;
    LONG WowTebOffset;
    PVOID ResourceRetValue;
    PVOID ReservedForWdf;
    ULONGLONG ReservedForCrt;
    GUID EffectiveContainerId;
    ULONGLONG LastSleepCounter;
    ULONG SpinCallCount;
#ifdef _WIN64
    UCHAR Padding8[4];
#endif
    ULONGLONG ExtendedFeatureDisableMask;
} TEB, *PTEB;

#if defined(_NTASSASSIN_NDK_EXTENSION_)

typedef struct _TEB64
{
    NT_TIB64 NtTib;
    WCHAR* POINTER_64 EnvironmentPointer;
    CLIENT_ID64 ClientId;
    VOID* POINTER_64 ActiveRpcHandle;
    VOID* POINTER_64 ThreadLocalStoragePointer;
    PEB64* POINTER_64 ProcessEnvironmentBlock;
    ULONG LastErrorValue;
    ULONG CountOfOwnedCriticalSections;
    VOID* POINTER_64 CsrClientThread;
    VOID* POINTER_64 Win32ThreadInfo;
    ULONG User32Reserved[26];
    ULONG UserReserved[5];
    VOID* POINTER_64 WOW32Reserved;
    ULONG CurrentLocale;
    ULONG FpSoftwareStatusRegister;
    VOID* POINTER_64 ReservedForDebuggerInstrumentation[16];
    VOID* POINTER_64 SystemReserved1[30];
    CHAR PlaceholderCompatibilityMode;
    UCHAR PlaceholderHydrationAlwaysExplicit;
    CHAR PlaceholderReserved[10];
    ULONG ProxiedProcessId;
    ACTIVATION_CONTEXT_STACK64 _ActivationStack;
    UCHAR WorkingOnBehalfTicket[8];
    LONG ExceptionCode;
    UCHAR Padding0[4];
    ACTIVATION_CONTEXT_STACK64* POINTER_64 ActivationContextStackPointer;
    VOID* POINTER_64 InstrumentationCallbackSp;
    VOID* POINTER_64 InstrumentationCallbackPreviousPc;
    VOID* POINTER_64 InstrumentationCallbackPreviousSp;
    ULONG TxFsContext;
    BOOLEAN InstrumentationCallbackDisabled;
    UCHAR UnalignedLoadStoreExceptions;
    UCHAR Padding1[2];
    GDI_TEB_BATCH64 GdiTebBatch;
    CLIENT_ID64 RealClientId;
    VOID* POINTER_64 GdiCachedProcessHandle;
    ULONG GdiClientPID;
    ULONG GdiClientTID;
    VOID* POINTER_64 GdiThreadLocalInfo;
    ULONGLONG Win32ClientInfo[62];
    VOID* POINTER_64 glDispatchTable[233];
    ULONGLONG glReserved1[29];
    VOID* POINTER_64 glReserved2;
    VOID* POINTER_64 glSectionInfo;
    VOID* POINTER_64 glSection;
    VOID* POINTER_64 glTable;
    VOID* POINTER_64 glCurrentRC;
    VOID* POINTER_64 glContext;
    ULONG LastStatusValue;
    UCHAR Padding2[4];
    UNICODE_STRING64 StaticUnicodeString;
    WCHAR StaticUnicodeBuffer[261];
    UCHAR Padding3[6];
    VOID* POINTER_64 DeallocationStack;
    VOID* POINTER_64 TlsSlots[64];
    LIST_ENTRY64 TlsLinks;
    VOID* POINTER_64 Vdm;
    VOID* POINTER_64 ReservedForNtRpc;
    VOID* POINTER_64 DbgSsReserved[2];
    ULONG HardErrorMode;
    UCHAR Padding4[4];
    VOID* POINTER_64 Instrumentation[11];
    GUID ActivityId;
    VOID* POINTER_64 SubProcessTag;
    VOID* POINTER_64 PerflibData;
    VOID* POINTER_64 EtwTraceData;
    VOID* POINTER_64 WinSockData;
    ULONG GdiBatchCount;
    union
    {
        PROCESSOR_NUMBER CurrentIdealProcessor;
        union
        {
            DWORD IdealProcessorValue;
            struct
            {
                UCHAR ReservedPad0;
                UCHAR ReservedPad1;
                UCHAR ReservedPad2;
                UCHAR IdealProcessor;
            };
        };
    };
    ULONG GuaranteedStackBytes;
    UCHAR Padding5[4];
    VOID* POINTER_64 ReservedForPerf;
    VOID* POINTER_64 ReservedForOle;
    ULONG WaitingOnLoaderLock;
    UCHAR Padding6[4];
    VOID* POINTER_64 SavedPriorityState;
    ULONGLONG ReservedForCodeCoverage;
    VOID* POINTER_64 ThreadPoolData;
    VOID* POINTER_64 TlsExpansionSlots;
    struct CHPEV2_CPUAREA_INFO* POINTER_64 ChpeV2CpuAreaInfo;
    VOID* POINTER_64 Unused;
    ULONG MuiGeneration;
    ULONG IsImpersonating;
    VOID* POINTER_64 NlsCache;
    VOID* POINTER_64 pShimData;
    ULONG HeapData;
    UCHAR Padding7[4];
    VOID* POINTER_64 CurrentTransactionHandle;
    TEB_ACTIVE_FRAME64* POINTER_64 ActiveFrame;
    VOID* POINTER_64 FlsData;
    VOID* POINTER_64 PreferredLanguages;
    VOID* POINTER_64 UserPrefLanguages;
    VOID* POINTER_64 MergedPrefLanguages;
    ULONG MuiImpersonation;
    union
    {
        USHORT CrossTebFlags;
        struct
        {
            USHORT SpareCrossTebBits : 16;
        };
    };
    union
    {
        USHORT SameTebFlags;
        struct
        {
            USHORT SafeThunkCall : 1;
            USHORT InDebugPrint : 1;
            USHORT HasFiberData : 1;
            USHORT SkipThreadAttach : 1;
            USHORT WerInShipAssertCode : 1;
            USHORT RanProcessInit : 1;
            USHORT ClonedThread : 1;
            USHORT SuppressDebugMsg : 1;
            USHORT DisableUserStackWalk : 1;
            USHORT RtlExceptionAttached : 1;
            USHORT InitialThread : 1;
            USHORT SessionAware : 1;
            USHORT LoadOwner : 1;
            USHORT LoaderWorker : 1;
            USHORT SkipLoaderInit : 1;
            USHORT SkipFileAPIBrokering : 1;
        };
    };
    VOID* POINTER_64 TxnScopeEnterCallback;
    VOID* POINTER_64 TxnScopeExitCallback;
    VOID* POINTER_64 TxnScopeContext;
    ULONG LockCount;
    LONG WowTebOffset;
    VOID* POINTER_64 ResourceRetValue;
    VOID* POINTER_64 ReservedForWdf;
    ULONGLONG ReservedForCrt;
    GUID EffectiveContainerId;
    ULONGLONG LastSleepCounter;
    ULONG SpinCallCount;
    UCHAR Padding8[4];
    ULONGLONG ExtendedFeatureDisableMask;
} TEB64, *PTEB64;

typedef struct _TEB32
{
    NT_TIB32 NtTib;
    WCHAR* POINTER_32 EnvironmentPointer;
    CLIENT_ID32 ClientId;
    VOID* POINTER_32 ActiveRpcHandle;
    VOID* POINTER_32 ThreadLocalStoragePointer;
    PEB32* POINTER_32 ProcessEnvironmentBlock;
    ULONG LastErrorValue;
    ULONG CountOfOwnedCriticalSections;
    VOID* POINTER_32 CsrClientThread;
    VOID* POINTER_32 Win32ThreadInfo;
    ULONG User32Reserved[26];
    ULONG UserReserved[5];
    VOID* POINTER_32 WOW32Reserved;
    ULONG CurrentLocale;
    ULONG FpSoftwareStatusRegister;
    VOID* POINTER_32 ReservedForDebuggerInstrumentation[16];
    VOID* POINTER_32 SystemReserved1[26];
    CHAR PlaceholderCompatibilityMode;
    UCHAR PlaceholderHydrationAlwaysExplicit;
    CHAR PlaceholderReserved[10];
    ULONG ProxiedProcessId;
    ACTIVATION_CONTEXT_STACK32 _ActivationStack;
    UCHAR WorkingOnBehalfTicket[8];
    LONG ExceptionCode;
    ACTIVATION_CONTEXT_STACK32* POINTER_32 ActivationContextStackPointer;
    VOID* POINTER_32 InstrumentationCallbackSp;
    VOID* POINTER_32 InstrumentationCallbackPreviousPc;
    VOID* POINTER_32 InstrumentationCallbackPreviousSp;
    UCHAR InstrumentationCallbackDisabled;
    UCHAR SpareBytes[23];
    ULONG TxFsContext;
    GDI_TEB_BATCH32 GdiTebBatch;
    CLIENT_ID32 RealClientId;
    VOID* POINTER_32 GdiCachedProcessHandle;
    ULONG GdiClientPID;
    ULONG GdiClientTID;
    VOID* POINTER_32 GdiThreadLocalInfo;
    ULONG Win32ClientInfo[62];
    VOID* POINTER_32 glDispatchTable[233];
    ULONG glReserved1[29];
    VOID* POINTER_32 glReserved2;
    VOID* POINTER_32 glSectionInfo;
    VOID* POINTER_32 glSection;
    VOID* POINTER_32 glTable;
    VOID* POINTER_32 glCurrentRC;
    VOID* POINTER_32 glContext;
    ULONG LastStatusValue;
    UNICODE_STRING32 StaticUnicodeString;
    WCHAR StaticUnicodeBuffer[261];
    VOID* POINTER_32 DeallocationStack;
    VOID* POINTER_32 TlsSlots[64];
    LIST_ENTRY32 TlsLinks;
    VOID* POINTER_32 Vdm;
    VOID* POINTER_32 ReservedForNtRpc;
    VOID* POINTER_32 DbgSsReserved[2];
    ULONG HardErrorMode;
    VOID* POINTER_32 Instrumentation[9];
    GUID ActivityId;
    VOID* POINTER_32 SubProcessTag;
    VOID* POINTER_32 PerflibData;
    VOID* POINTER_32 EtwTraceData;
    VOID* POINTER_32 WinSockData;
    ULONG GdiBatchCount;
    union
    {
        PROCESSOR_NUMBER CurrentIdealProcessor;
        union
        {
            DWORD IdealProcessorValue;
            struct
            {
                UCHAR ReservedPad0;
                UCHAR ReservedPad1;
                UCHAR ReservedPad2;
                UCHAR IdealProcessor;
            };
        };
    };
    ULONG GuaranteedStackBytes;
    VOID* POINTER_32 ReservedForPerf;
    VOID* POINTER_32 ReservedForOle;
    ULONG WaitingOnLoaderLock;
    VOID* POINTER_32 SavedPriorityState;
    ULONG ReservedForCodeCoverage;
    VOID* POINTER_32 ThreadPoolData;
    VOID* POINTER_32 TlsExpansionSlots;
    ULONG MuiGeneration;
    ULONG IsImpersonating;
    VOID* POINTER_32 NlsCache;
    VOID* POINTER_32 pShimData;
    ULONG HeapData;
    VOID* POINTER_32 CurrentTransactionHandle;
    TEB_ACTIVE_FRAME32* POINTER_32 ActiveFrame;
    VOID* POINTER_32 FlsData;
    VOID* POINTER_32 PreferredLanguages;
    VOID* POINTER_32 UserPrefLanguages;
    VOID* POINTER_32 MergedPrefLanguages;
    ULONG MuiImpersonation;
    union
    {
        USHORT CrossTebFlags;
        struct
        {
            USHORT SpareCrossTebBits : 16;
        };
    };
    union
    {
        USHORT SameTebFlags;
        struct
        {
            USHORT SafeThunkCall : 1;
            USHORT InDebugPrint : 1;
            USHORT HasFiberData : 1;
            USHORT SkipThreadAttach : 1;
            USHORT WerInShipAssertCode : 1;
            USHORT RanProcessInit : 1;
            USHORT ClonedThread : 1;
            USHORT SuppressDebugMsg : 1;
            USHORT DisableUserStackWalk : 1;
            USHORT RtlExceptionAttached : 1;
            USHORT InitialThread : 1;
            USHORT SessionAware : 1;
            USHORT LoadOwner : 1;
            USHORT LoaderWorker : 1;
            USHORT SkipLoaderInit : 1;
            USHORT SkipFileAPIBrokering : 1;
        };
    };
    VOID* POINTER_32 TxnScopeEnterCallback;
    VOID* POINTER_32 TxnScopeExitCallback;
    VOID* POINTER_32 TxnScopeContext;
    ULONG LockCount;
    LONG WowTebOffset;
    VOID* POINTER_32 ResourceRetValue;
    VOID* POINTER_32 ReservedForWdf;
    ULONGLONG ReservedForCrt;
    GUID EffectiveContainerId;
    ULONGLONG LastSleepCounter;
    ULONG SpinCallCount;
    ULONGLONG ExtendedFeatureDisableMask;
} TEB32, *PTEB32;

#endif /* defined(_NTASSASSIN_NDK_EXTENSION_) */

#pragma endregion
