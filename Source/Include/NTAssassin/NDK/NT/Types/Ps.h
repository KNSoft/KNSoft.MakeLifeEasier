#pragma once

#include "../MinDef.h"

#include "Ke.h"

typedef enum _PROCESSINFOCLASS
{
    ProcessBasicInformation = 0,
    ProcessQuotaLimits = 1,
    ProcessIoCounters = 2,
    ProcessVmCounters = 3,
    ProcessTimes = 4,
    ProcessBasePriority = 5,
    ProcessRaisePriority = 6,
    ProcessDebugPort = 7,
    ProcessExceptionPort = 8,
    ProcessAccessToken = 9,
    ProcessLdtInformation = 10,
    ProcessLdtSize = 11,
    ProcessDefaultHardErrorMode = 12,
    ProcessIoPortHandlers = 13, // Note: this is kernel mode only
    ProcessPooledUsageAndLimits = 14,
    ProcessWorkingSetWatch = 15,
    ProcessUserModeIOPL = 16,
    ProcessEnableAlignmentFaultFixup = 17,
    ProcessPriorityClass = 18,
    ProcessWx86Information = 19,
    ProcessHandleCount = 20,
    ProcessAffinityMask = 21,
    ProcessPriorityBoost = 22,
    ProcessDeviceMap = 23,
    ProcessSessionInformation = 24,
    ProcessForegroundInformation = 25,
    ProcessWow64Information = 26,
    ProcessImageFileName = 27,
    ProcessLUIDDeviceMapsEnabled = 28,
    ProcessBreakOnTermination = 29,
    ProcessDebugObjectHandle = 30,
    ProcessDebugFlags = 31,
    ProcessHandleTracing = 32,
    ProcessIoPriority = 33,
    ProcessExecuteFlags = 34,
    ProcessTlsInformation = 35,
    ProcessCookie = 36,
    ProcessImageInformation = 37,
    ProcessCycleTime = 38,
    ProcessPagePriority = 39,
    ProcessInstrumentationCallback = 40,
    ProcessThreadStackAllocation = 41,
    ProcessWorkingSetWatchEx = 42,
    ProcessImageFileNameWin32 = 43,
    ProcessImageFileMapping = 44,
    ProcessAffinityUpdateMode = 45,
    ProcessMemoryAllocationMode = 46,
    ProcessGroupInformation = 47,
    ProcessTokenVirtualizationEnabled = 48,
    ProcessOwnerInformation = 49,
    ProcessWindowInformation = 50,
    ProcessHandleInformation = 51,
    ProcessMitigationPolicy = 52,
    ProcessDynamicFunctionTableInformation = 53,
    ProcessHandleCheckingMode = 54,
    ProcessKeepAliveCount = 55,
    ProcessRevokeFileHandles = 56,
    ProcessWorkingSetControl = 57,
    ProcessHandleTable = 58,
    ProcessCheckStackExtentsMode = 59,
    ProcessCommandLineInformation = 60,
    ProcessProtectionInformation = 61,
    ProcessMemoryExhaustion = 62,
    ProcessFaultInformation = 63,
    ProcessTelemetryIdInformation = 64,
    ProcessCommitReleaseInformation = 65,
    ProcessReserved1Information = 66,
    ProcessReserved2Information = 67,
    ProcessSubsystemProcess = 68,
    ProcessInPrivate = 70,
    ProcessRaiseUMExceptionOnInvalidHandleClose = 71,
    ProcessSubsystemInformation = 75,
    ProcessWin32kSyscallFilterInformation = 79,
    ProcessEnergyTrackingState = 82,
    MaxProcessInfoClass // MaxProcessInfoClass should always be the last enum
} PROCESSINFOCLASS;

typedef struct _PROCESS_BASIC_INFORMATION
{
    NTSTATUS ExitStatus;
    PVOID PebBaseAddress; // PPEB
    ULONG_PTR AffinityMask;
    KPRIORITY BasePriority;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

typedef enum _THREADINFOCLASS
{
    ThreadBasicInformation = 0,
    ThreadTimes = 1,
    ThreadPriority = 2,
    ThreadBasePriority = 3,
    ThreadAffinityMask = 4,
    ThreadImpersonationToken = 5,
    ThreadDescriptorTableEntry = 6,
    ThreadEnableAlignmentFaultFixup = 7,
    ThreadEventPair_Reusable = 8,
    ThreadQuerySetWin32StartAddress = 9,
    ThreadZeroTlsCell = 10,
    ThreadPerformanceCount = 11,
    ThreadAmILastThread = 12,
    ThreadIdealProcessor = 13,
    ThreadPriorityBoost = 14,
    ThreadSetTlsArrayAddress = 15, // Obsolete
    ThreadIsIoPending = 16,
    ThreadHideFromDebugger = 17,
    ThreadBreakOnTermination = 18,
    ThreadSwitchLegacyState = 19,
    ThreadIsTerminated = 20,
    ThreadLastSystemCall = 21,
    ThreadIoPriority = 22,
    ThreadCycleTime = 23,
    ThreadPagePriority = 24,
    ThreadActualBasePriority = 25,
    ThreadTebInformation = 26,
    ThreadCSwitchMon = 27, // Obsolete
    ThreadCSwitchPmu = 28,
    ThreadWow64Context = 29,
    ThreadGroupInformation = 30,
    ThreadUmsInformation = 31, // Obsolete
    ThreadCounterProfiling = 32,
    ThreadIdealProcessorEx = 33,
    ThreadCpuAccountingInformation = 34,
    ThreadSuspendCount = 35,
    ThreadNameInformation = 38,
    ThreadActualGroupAffinity = 41,
    ThreadDynamicCodePolicyInfo = 42,
    ThreadSubsystemInformation = 45,

    MaxThreadInfoClass = 53,
} THREADINFOCLASS;

typedef struct _THREAD_BASIC_INFORMATION
{
    NTSTATUS ExitStatus;
    PVOID TebBaseAddress; //PTEB
    CLIENT_ID ClientId;
    KAFFINITY AffinityMask;
    KPRIORITY Priority;
    KPRIORITY BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;
