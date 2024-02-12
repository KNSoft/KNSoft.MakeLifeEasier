#pragma once

#include "../NT/MinDef.h"
#include "../NT/Types.h"

#include <minwinbase.h>

EXTERN_C_START

#pragma region Csr*

NTSYSAPI
HANDLE
NTAPI
CsrGetProcessId();

#pragma endregion

#pragma region Ldr*

NTSTATUS
NTAPI
LdrLockLoaderLock(
    _In_ ULONG Flags,
    _Out_opt_ PULONG Disposition,
    _Out_opt_ PULONG_PTR Cookie);

NTSTATUS
NTAPI
LdrUnlockLoaderLock(
    _In_ ULONG Flags,
    _In_opt_ ULONG Cookie);

NTSTATUS
NTAPI
LdrDisableThreadCalloutsForDll(
    _In_ PVOID BaseAddress);

NTSTATUS
NTAPI
LdrFindEntryForAddress(
    _In_ PVOID Address,
    _Out_ PLDR_DATA_TABLE_ENTRY* Module);

NTSTATUS
NTAPI
LdrEnumerateLoadedModules(
    _Reserved_ ULONG ReservedFlag,
    _In_ PLDR_ENUM_CALLBACK EnumProc,
    _In_opt_ PVOID Context);

NTSTATUS
NTAPI
LdrGetDllHandle(
    _In_opt_ PWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PUNICODE_STRING DllName,
    _Out_ PVOID* DllHandle);

NTSYSAPI
NTSTATUS
NTAPI
LdrLoadDll(
    _In_opt_ PWSTR DllSearchPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PUNICODE_STRING DllName,
    _Out_ PVOID* BaseAddress);

NTSYSAPI
NTSTATUS
NTAPI
LdrUnloadDll(
    _In_ PVOID* BaseAddress);

NTSYSAPI
NTSTATUS
NTAPI
LdrGetProcedureAddress(
    _In_ PVOID BaseAddress,
    _In_opt_ _When_(Ordinal == 0, _Notnull_) PANSI_STRING Name,
    _In_opt_ _When_(Name == NULL, _In_range_(> , 0)) ULONG Ordinal,
    _Out_ PVOID* ProcedureAddress);

NTSYSAPI
NTSTATUS
NTAPI
LdrRegisterDllNotification(
    _In_ ULONG Flags,
    _In_ PLDR_DLL_NOTIFICATION_FUNCTION NotificationFunction,
    _In_opt_ PVOID Context,
    _Out_ PVOID *Cookie);

NTSYSAPI
NTSTATUS
NTAPI
LdrUnregisterDllNotification(
  _In_ PVOID Cookie);

#pragma endregion

#pragma region Dbg*

NTSYSAPI
ULONG
__cdecl
DbgPrint(
    _In_z_ _Printf_format_string_ PCSTR Format,
    ...);

NTSYSAPI
ULONG
__cdecl
DbgPrintEx(
    _In_ ULONG ComponentId,
    _In_ ULONG Level,
    _In_z_ _Printf_format_string_ PCSTR Format,
    ...);

NTSYSAPI
ULONG
NTAPI
vDbgPrintEx(
    _In_ ULONG ComponentId,
    _In_ ULONG Level,
    _In_z_ PCCH Format,
    _In_ va_list arglist);

NTSYSAPI
ULONG
NTAPI
vDbgPrintExWithPrefix(
    _In_z_ PCCH Prefix,
    _In_ ULONG ComponentId,
    _In_ ULONG Level,
    _In_z_ PCCH Format,
    _In_ va_list arglist);

NTSYSAPI
ULONG
__cdecl
DbgPrintReturnControlC(
    _In_z_ _Printf_format_string_ PCCH Format,
    ...);

NTSYSAPI
NTSTATUS
NTAPI
DbgQueryDebugFilterState(
    _In_ ULONG ComponentId,
    _In_ ULONG Level);

NTSYSAPI
NTSTATUS
NTAPI
DbgSetDebugFilterState(
    _In_ ULONG ComponentId,
    _In_ ULONG Level,
    _In_ BOOLEAN State);

#pragma endregion

#pragma region Rtl*

#pragma region String

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
LONG
NTAPI
RtlCompareString(
    _In_ const STRING* String1,
    _In_ const STRING* String2,
    _In_ BOOLEAN CaseInSensitive);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlEqualString(
    _In_ const STRING* String1,
    _In_ const STRING* String2,
    _In_ BOOLEAN CaseInSensitive);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToInteger(
    _In_ PCUNICODE_STRING String,
    _In_opt_ ULONG Base,
    _Out_ PULONG Value);

_IRQL_requires_max_(PASSIVE_LEVEL)
_At_(String->MaximumLength, _Const_)
NTSYSAPI
NTSTATUS
NTAPI
RtlIntegerToUnicodeString(
    _In_ ULONG Value,
    _In_opt_ ULONG Base,
    _Inout_ PUNICODE_STRING String);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlInitAnsiString(
    _Out_ PANSI_STRING DestinationString,
    _In_opt_z_ __drv_aliasesMem PCSZ SourceString);

_IRQL_requires_max_(DISPATCH_LEVEL)
_At_(DestinationString->Buffer, _Post_equal_to_(SourceString))
_At_(DestinationString->Length, _Post_equal_to_(_String_length_(SourceString) * sizeof(WCHAR)))
_At_(DestinationString->MaximumLength, _Post_equal_to_((_String_length_(SourceString) + 1) * sizeof(WCHAR)))
NTSYSAPI
VOID
NTAPI
RtlInitUnicodeString(
    _Out_ PUNICODE_STRING DestinationString,
    _In_opt_z_ __drv_aliasesMem PCWSTR SourceString);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlAnsiStringToUnicodeString(
    _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
    _When_(!AllocateDestinationString, _Inout_)
    PUNICODE_STRING DestinationString,
    _In_ PCANSI_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString);

_When_(AllocateDestinationString,
       _At_(DestinationString->MaximumLength,
            _Out_range_(<= , (SourceString->MaximumLength / sizeof(WCHAR))))
)
_When_(!AllocateDestinationString,
       _At_(DestinationString->Buffer, _Const_)
       _At_(DestinationString->MaximumLength, _Const_)
)
_IRQL_requires_max_(PASSIVE_LEVEL)
_When_(AllocateDestinationString, _Must_inspect_result_)
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToAnsiString(
    _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
    _When_(!AllocateDestinationString, _Inout_)
    PANSI_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
LONG
NTAPI
RtlCompareUnicodeStrings(
    _In_reads_(String1Length) PCWCH String1,
    _In_ SIZE_T String1Length,
    _In_reads_(String2Length) PCWCH String2,
    _In_ SIZE_T String2Length,
    _In_ BOOLEAN CaseInSensitive);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
LONG
NTAPI
RtlCompareUnicodeString(
    _In_ PCUNICODE_STRING String1,
    _In_ PCUNICODE_STRING String2,
    _In_ BOOLEAN CaseInSensitive);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlEqualUnicodeString(
    _In_ PCUNICODE_STRING String1,
    _In_ PCUNICODE_STRING String2,
    _In_ BOOLEAN CaseInSensitive);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlInitUnicodeStringEx(
    _Out_ PUNICODE_STRING DestinationString,
    _In_opt_z_ __drv_aliasesMem PCWSTR SourceString);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlInitAnsiStringEx(
    _Out_ PANSI_STRING DestinationString,
    _In_opt_z_ __drv_aliasesMem PCSZ SourceString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlMultiByteToUnicodeN(
    _Out_writes_bytes_to_(MaxBytesInUnicodeString, *BytesInUnicodeString) PWCH UnicodeString,
    _In_ ULONG MaxBytesInUnicodeString,
    _Out_opt_ PULONG BytesInUnicodeString,
    _In_reads_bytes_(BytesInMultiByteString) const CHAR* MultiByteString,
    _In_ ULONG BytesInMultiByteString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToMultiByteN(
    _Out_writes_bytes_to_(MaxBytesInMultiByteString, *BytesInMultiByteString) PCHAR MultiByteString,
    _In_ ULONG MaxBytesInMultiByteString,
    _Out_opt_ PULONG BytesInMultiByteString,
    _In_reads_bytes_(BytesInUnicodeString) PCWCH UnicodeString,
    _In_ ULONG BytesInUnicodeString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlFreeUnicodeString(
    _Inout_ _At_(UnicodeString->Buffer, _Frees_ptr_opt_) PUNICODE_STRING UnicodeString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlFreeAnsiString(
    _Inout_ _At_(AnsiString->Buffer, _Frees_ptr_opt_) PANSI_STRING AnsiString);

#pragma endregion

#pragma region Process and Thread

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateUserThread(
    _In_ HANDLE hProcess,
    _In_opt_ PSECURITY_DESCRIPTOR ThreadSecurityDescriptor,
    _In_ BOOLEAN CreateSuspended,
    _In_opt_ ULONG ZeroBits,
    _In_opt_ SIZE_T MaximumStackSize,
    _In_opt_ SIZE_T CommittedStackSize,
    _In_ LPTHREAD_START_ROUTINE StartAddress,
    _In_opt_ PVOID Parameter,
    _Out_opt_ PHANDLE hThread,
    _Out_opt_ PCLIENT_ID ClientId);

NTSYSAPI
VOID
NTAPI
RtlExitUserThread(
    _In_ NTSTATUS ExitStatus);

typedef ULONG
(NTAPI* RTLP_UNHANDLED_EXCEPTION_FILTER)(
    _In_ PEXCEPTION_POINTERS ExceptionInfo);
typedef RTLP_UNHANDLED_EXCEPTION_FILTER* PRTLP_UNHANDLED_EXCEPTION_FILTER;

NTSYSAPI
VOID
NTAPI
RtlSetUnhandledExceptionFilter(
    _In_opt_ PRTLP_UNHANDLED_EXCEPTION_FILTER TopLevelExceptionFilter);

NTSYSAPI
VOID
DECLSPEC_NORETURN
NTAPI
RtlExitUserProcess(
    _In_ NTSTATUS ExitStatus);

#pragma endregion

#pragma region Critical Section

NTSYSAPI
VOID
NTAPI
RtlInitializeCriticalSection(
    _Out_ PRTL_CRITICAL_SECTION lpCriticalSection);

NTSYSAPI
VOID
NTAPI
RtlEnterCriticalSection(
    _Inout_ PRTL_CRITICAL_SECTION lpCriticalSection);

NTSYSAPI
VOID
NTAPI
RtlLeaveCriticalSection(
    _Inout_ PRTL_CRITICAL_SECTION lpCriticalSection);

NTSYSAPI
VOID
NTAPI
RtlDeleteCriticalSection(
    _Inout_ PRTL_CRITICAL_SECTION lpCriticalSection);

#pragma endregion

#pragma region One-Time initialization

NTSYSAPI
VOID
NTAPI
RtlRunOnceInitialize(
    _Out_ PRTL_RUN_ONCE RunOnce);

_Maybe_raises_SEH_exception_
NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceExecuteOnce(
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _In_ __callback PRTL_RUN_ONCE_INIT_FN InitFn,
    _Inout_opt_ PVOID Parameter,
    _Outptr_opt_result_maybenull_ PVOID* Context);

_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceBeginInitialize(
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _In_ ULONG Flags,
    _Outptr_opt_result_maybenull_ PVOID* Context);

NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceComplete(
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _In_ ULONG Flags,
    _In_opt_ PVOID Context);

#pragma endregion

#pragma region SRW Lock

NTSYSAPI
VOID
NTAPI
RtlInitializeSRWLock(
    _Out_ PRTL_SRWLOCK SRWLock);

NTSYSAPI
VOID
NTAPI
RtlAcquireSRWLockExclusive(
    _Inout_ PRTL_SRWLOCK SRWLock);

NTSYSAPI
VOID
NTAPI
RtlAcquireSRWLockShared(
    _Inout_ PRTL_SRWLOCK SRWLock);

NTSYSAPI
VOID
NTAPI
RtlReleaseSRWLockExclusive(
    _Inout_ PRTL_SRWLOCK SRWLock);

NTSYSAPI
VOID
NTAPI
RtlReleaseSRWLockShared(
    _Inout_ PRTL_SRWLOCK SRWLock);

NTSYSAPI
BOOLEAN
NTAPI
RtlTryAcquireSRWLockExclusive(
    _Inout_ PRTL_SRWLOCK SRWLock);

NTSYSAPI
BOOLEAN
NTAPI
RtlTryAcquireSRWLockShared(
    _Inout_ PRTL_SRWLOCK SRWLock);

#pragma endregion

#pragma region Path

NTSYSAPI
BOOLEAN
NTAPI
RtlDosPathNameToNtPathName_U(
    _In_opt_z_ LPCWSTR DosName,
    _Out_ PUNICODE_STRING NtName,
    _Out_opt_ LPCWSTR* PartName,
    _Out_opt_ PRTL_RELATIVE_NAME_U RelativeName);

NTSYSAPI
NTSTATUS
NTAPI
RtlDosPathNameToNtPathName_U_WithStatus(
    _In_opt_z_ LPCWSTR DosName,
    _Out_ PUNICODE_STRING NtName,
    _Out_opt_ LPCWSTR* PartName,
    _Out_opt_ PRTL_RELATIVE_NAME_U RelativeName);

NTSYSAPI
VOID
NTAPI
RtlReleaseRelativeName(
    _In_ PRTL_RELATIVE_NAME_U RelativeName);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetCurrentDirectory_U(
    _In_ PUNICODE_STRING name);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetEnvironmentVariable(
    _In_z_ PWSTR* Environment,
    _In_ PUNICODE_STRING Name,
    _In_ PUNICODE_STRING Value);

#pragma endregion

#pragma region Bitmap

NTSYSAPI
VOID
NTAPI
RtlInitializeBitMap(
    _Out_ PRTL_BITMAP BitMapHeader,
    _In_opt_ __drv_aliasesMem PULONG BitMapBuffer,
    _In_opt_ ULONG SizeOfBitMap);

#pragma endregion

#pragma region Heap

_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlCreateHeap(
    _In_ ULONG Flags,
    _In_opt_ PVOID HeapBase,
    _In_opt_ SIZE_T ReserveSize,
    _In_opt_ SIZE_T CommitSize,
    _In_opt_ PVOID Lock,
    _When_((Flags & 0x100) != 0,
           _In_reads_bytes_opt_(sizeof(RTL_SEGMENT_HEAP_PARAMETERS)))
    _When_((Flags & 0x100) == 0,
           _In_reads_bytes_opt_(sizeof(RTL_HEAP_PARAMETERS)))
    PRTL_HEAP_PARAMETERS Parameters);

NTSYSAPI
ULONG
NTAPI
RtlCreateTagHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags,
    _In_opt_ PWSTR TagName,
    _In_ PWSTR TagSubName);

NTSYSAPI
PWSTR
NTAPI
RtlQueryTagHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags,
    _In_ USHORT TagIndex,
    _In_ BOOLEAN ResetCounters,
    _Out_ PRTL_HEAP_TAG_INFO TagInfo);

_Success_(return != 0)
NTSYSAPI
BOOLEAN
NTAPI
RtlFreeHeap(
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG Flags,
    _Frees_ptr_opt_ PVOID BaseAddress);

NTSYSAPI
PVOID
NTAPI
RtlDestroyHeap(
    _In_ _Post_invalid_ PVOID HeapHandle);

NTSYSAPI
_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
PVOID
NTAPI
RtlAllocateHeap(
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG Flags,
    _In_ SIZE_T Size);

NTSYSAPI
_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
PVOID
NTAPI
RtlReAllocateHeap(
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG Flags,
    _Frees_ptr_opt_ PVOID BaseAddress,
    _In_ SIZE_T Size);

NTSYSAPI
SIZE_T
NTAPI
RtlSizeHeap(
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG Flags,
    _In_ PVOID BaseAddress);

#pragma endregion

#pragma region LUID

//
// BOOLEAN
// RtlEqualLuid(
//      PLUID L1,
//      PLUID L2
//      );

#define RtlEqualLuid(L1, L2) (((L1)->LowPart == (L2)->LowPart) && \
                              ((L1)->HighPart  == (L2)->HighPart))

//
// BOOLEAN
// RtlIsZeroLuid(
//      PLUID L1
//      );
//
#define RtlIsZeroLuid(L1) ((BOOLEAN) (((L1)->LowPart | (L1)->HighPart) == 0))

FORCEINLINE
LUID
NTAPI_INLINE
RtlConvertLongToLuid(_In_ LONG Long)
{
    LUID TempLuid;
    LARGE_INTEGER TempLi;

    TempLi.QuadPart = Long;
    TempLuid.LowPart = TempLi.u.LowPart;
    TempLuid.HighPart = TempLi.u.HighPart;
    return(TempLuid);
}

FORCEINLINE
LUID
NTAPI_INLINE
RtlConvertUlongToLuid(_In_ ULONG Ulong)
{
    LUID TempLuid;

    TempLuid.LowPart = Ulong;
    TempLuid.HighPart = 0;
    return(TempLuid);
}

#pragma endregion

#pragma region Random

_IRQL_requires_max_(APC_LEVEL)
_Ret_range_(<= , MAXLONG)
NTSYSAPI
ULONG
NTAPI
RtlRandom(
    _Inout_ PULONG Seed);

_IRQL_requires_max_(APC_LEVEL)
_Ret_range_(<= , MAXLONG)
NTSYSAPI
ULONG
NTAPI
RtlRandomEx(
    _Inout_ PULONG Seed);

#pragma endregion

#pragma region Error and Exception

_IRQL_requires_max_(APC_LEVEL)
_When_(Status < 0, _Out_range_(> , 0))
_When_(Status >= 0, _Out_range_(== , 0))
NTSYSAPI
ULONG
NTAPI
RtlNtStatusToDosError(
   _In_ NTSTATUS Status);

_When_(Status < 0, _Out_range_(> , 0))
_When_(Status >= 0, _Out_range_(== , 0))
NTSYSAPI
ULONG
NTAPI
RtlNtStatusToDosErrorNoTeb(
   _In_ NTSTATUS Status);

NTSYSAPI
VOID
NTAPI
RtlRaiseStatus(
    _In_ NTSTATUS Status);

#pragma endregion

#pragma region Uncategorized

NTSYSAPI
NTSTATUS
NTAPI
RtlFindMessage(
    _In_ PVOID BaseAddress,
    _In_ ULONG Type,
    _In_ ULONG Language,
    _In_ ULONG MessageId,
    _Out_ PMESSAGE_RESOURCE_ENTRY * MessageResourceEntry);

NTSYSAPI
NTSTATUS
NTAPI
RtlWow64EnableFsRedirectionEx(
    _In_ PVOID Wow64FsEnableRedirection,
    _Out_ PVOID * OldFsRedirectionLevel);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetUserPreferredUILanguages(
    _In_ ULONG Flags,
    _In_opt_ PCWSTR LocaleName,
    _Out_ PULONG NumLanguages,
    _Out_writes_opt_z_(*LanguagesCchSize) PWSTR LanguagesBuffer,
    _Inout_ PULONG LanguagesCchSize);

#pragma endregion

#pragma endregion

#pragma region Nt*

#pragma region Object

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryObject(
    _In_opt_ HANDLE Handle,
    _In_ OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _Out_writes_bytes_opt_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength,
    _Out_opt_ PULONG ReturnLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtClose(
    _In_ _Post_ptr_invalid_ HANDLE Handle);

#pragma endregion

#pragma region Virtual Memory

NTSYSAPI
NTSTATUS
NTAPI
NtProtectVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _Inout_ _At_(*BaseAddress,
                 _Readable_bytes_(*RegionSize)
                 _Writable_bytes_(*RegionSize)
                 _Post_readable_byte_size_(*RegionSize)) PVOID * BaseAddress,
    _Inout_ PSIZE_T RegionSize,
    _In_ ULONG Protect,
    _Out_ PULONG OldProtect);

NTSYSAPI
NTSTATUS
NTAPI
NtReadVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _Out_ PVOID Buffer,
    _In_ SIZE_T NumberOfBytesToRead,
    _Out_opt_ PSIZE_T NumberOfBytesRead);

NTSYSAPI
NTSTATUS
NTAPI
NtWriteVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ PVOID Buffer,
    _In_ SIZE_T NumberOfBytesToWrite,
    _Out_opt_ PSIZE_T NumberOfBytesWritten);

_Must_inspect_result_
__drv_allocatesMem(Mem)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAllocateVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _Inout_ _At_(*BaseAddress,
                 _Readable_bytes_(*RegionSize)
                 _Writable_bytes_(*RegionSize)
                 _Post_readable_byte_size_(*RegionSize)) PVOID * BaseAddress,
    _In_ ULONG_PTR ZeroBits,
    _Inout_ PSIZE_T RegionSize,
    _In_ ULONG AllocationType,
    _In_ ULONG Protect);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFreeVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _Inout_ __drv_freesMem(Mem) PVOID * BaseAddress,
    _Inout_ PSIZE_T RegionSize,
    _In_ ULONG FreeType);

_Must_inspect_result_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _In_ MEMORY_INFORMATION_CLASS MemoryInformationClass,
    _Out_writes_bytes_(MemoryInformationLength) PVOID MemoryInformation,
    _In_ SIZE_T MemoryInformationLength,
    _Out_opt_ PSIZE_T ReturnLength);

#pragma endregion

#pragma region Process and Thread

NTSYSAPI
NTSTATUS
NTAPI
NtOpenThread(
    _Out_ PHANDLE ThreadHandle,
    _In_  ACCESS_MASK DesiredAccess,
    _In_  POBJECT_ATTRIBUTES ObjectAttributes,
    _In_  PCLIENT_ID ClientId);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PCLIENT_ID ClientId);

NTSYSAPI
NTSTATUS
NTAPI
NtSuspendThread(
    _In_ HANDLE ThreadHandle,
    _Out_opt_ PULONG SuspendCount);

NTSYSAPI
NTSTATUS
NTAPI
NtResumeThread(
    _In_ HANDLE ThreadHandle,
    _Out_opt_ PULONG SuspendCount);

NTSYSAPI
NTSTATUS
NTAPI
NtQueryInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ THREADINFOCLASS ThreadInformationClass,
    _Out_writes_bytes_to_(ThreadInformationLength, *ReturnLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
    _Out_opt_ PULONG ReturnLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ THREADINFOCLASS ThreadInformationClass,
    _When_((ThreadInformationClass != ThreadManageWritesToExecutableMemory),
           _In_reads_bytes_(ThreadInformationLength))
    _When_((ThreadInformationClass == ThreadManageWritesToExecutableMemory),
           _Inout_updates_(ThreadInformationLength))
    PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength);

NTSYSAPI
NTSTATUS
NTAPI
NtQueryInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _Out_writes_bytes_to_(ProcessInformationLength, *ReturnLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Out_opt_ PULONG ReturnLength);

NTSYSAPI
NTSTATUS
NTAPI
NtFlushInstructionCache(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _In_ SIZE_T FlushSize);

NTSYSAPI
NTSTATUS
NTAPI
NtGetContextThread(
    _In_ HANDLE ThreadHandle,
    _Inout_ PCONTEXT ThreadContext);

NTSYSAPI
NTSTATUS
NTAPI
NtSetContextThread(
    _In_ HANDLE ThreadHandle,
    _In_ PCONTEXT ThreadContext);

NTSYSAPI
NTSTATUS
NTAPI
NtTerminateProcess(
    _In_opt_ HANDLE ProcessHandle,
    _In_ NTSTATUS ExitStatus);

NTSYSAPI
NTSTATUS
NTAPI
NtDelayExecution(
    _In_ BOOLEAN Alertable,
    _In_ PLARGE_INTEGER DelayInterval);

NTSYSAPI
NTSTATUS
NTAPI
NtWaitForSingleObject(
    _In_ HANDLE Handle,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout);

#pragma endregion

#pragma region File

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateFile(
    _Out_ PHANDLE FileHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
    _In_ ULONG EaLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReadFile(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_opt_ PLARGE_INTEGER ByteOffset,
    _In_opt_ PULONG Key);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWriteFile(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_opt_ PLARGE_INTEGER ByteOffset,
    _In_opt_ PULONG Key);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationFile(
    _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_writes_bytes_(Length) PVOID FileInformation,
    _In_ ULONG Length,
    _In_ FILE_INFORMATION_CLASS FileInformationClass);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationFile(
    _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_reads_bytes_(Length) PVOID FileInformation,
    _In_ ULONG Length,
    _In_ FILE_INFORMATION_CLASS FileInformationClass);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeviceIoControlFile(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFsControlFile(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG FsControlCode,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenFile(
    _Out_ PHANDLE FileHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG ShareAccess,
    _In_ ULONG OpenOptions);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryDirectoryFile(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_writes_bytes_(Length) PVOID FileInformation,
    _In_ ULONG Length,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _In_ BOOLEAN ReturnSingleEntry,
    _In_opt_ PUNICODE_STRING FileName,
    _In_ BOOLEAN RestartScan);

NTSYSAPI
NTSTATUS
NTAPI
NtQueryAttributesFile(
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PFILE_BASIC_INFORMATION FileInformation);

NTSYSAPI
NTSTATUS
NTAPI
NtQueryFullAttributesFile(
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION FileInformation);

#pragma endregion

#pragma region Section

_Must_inspect_result_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateSection(
    _Out_ PHANDLE SectionHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PLARGE_INTEGER MaximumSize,
    _In_ ULONG SectionPageProtection,
    _In_ ULONG AllocationAttributes,
    _In_opt_ HANDLE FileHandle);

_Must_inspect_result_
_Post_satisfies_(*ViewSize >= _Old_(*ViewSize))
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
NtMapViewOfSection(
    _In_ HANDLE SectionHandle,
    _In_ HANDLE ProcessHandle,
    _Outptr_result_bytebuffer_(*ViewSize) PVOID * BaseAddress,
    _In_ ULONG_PTR ZeroBits,
    _In_ SIZE_T CommitSize,
    _Inout_opt_ PLARGE_INTEGER SectionOffset,
    _Inout_ PSIZE_T ViewSize,
    _In_ SECTION_INHERIT InheritDisposition,
    _In_ ULONG AllocationType,
    _In_ ULONG Win32Protect);

NTSYSAPI
NTSTATUS
NTAPI
NtUnmapViewOfSection(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress);

#pragma endregion

#pragma region Token

NTSYSAPI
NTSTATUS
NTAPI
NtCreateToken(
    _Out_ PHANDLE TokenHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ TOKEN_TYPE TokenType,
    _In_ PLUID AuthenticationId,
    _In_ PLARGE_INTEGER ExpirationTime,
    _In_ PTOKEN_USER TokenUser,
    _In_ PTOKEN_GROUPS TokenGroups,
    _In_ PTOKEN_PRIVILEGES TokenPrivileges,
    _In_opt_ PTOKEN_OWNER TokenOwner,
    _In_ PTOKEN_PRIMARY_GROUP TokenPrimaryGroup,
    _In_opt_ PTOKEN_DEFAULT_DACL TokenDefaultDacl,
    _In_ PTOKEN_SOURCE TokenSource);

_Must_inspect_result_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcessToken(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE TokenHandle);

_When_(TokenInformationClass == TokenAccessInformation,
       _At_(TokenInformationLength, _In_range_(>= , sizeof(TOKEN_ACCESS_INFORMATION)))
)
_Must_inspect_result_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationToken(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
    _Out_writes_bytes_to_opt_(TokenInformationLength, *ReturnLength) PVOID TokenInformation,
    _In_ ULONG TokenInformationLength,
    _Out_ PULONG ReturnLength);

_Must_inspect_result_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationToken(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
    _In_reads_bytes_(TokenInformationLength) PVOID TokenInformation,
    _In_ ULONG TokenInformationLength);

_Must_inspect_result_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAdjustPrivilegesToken(
    _In_ HANDLE TokenHandle,
    _In_ BOOLEAN DisableAllPrivileges,
    _In_opt_ PTOKEN_PRIVILEGES NewState,
    _In_ ULONG BufferLength,
    _Out_writes_bytes_to_opt_(BufferLength, *ReturnLength) PTOKEN_PRIVILEGES PreviousState,
    _Out_ _When_(PreviousState == NULL, _Out_opt_) PULONG ReturnLength);

_Must_inspect_result_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDuplicateToken(
    _In_ HANDLE ExistingTokenHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ BOOLEAN EffectiveOnly,
    _In_ TOKEN_TYPE TokenType,
    _Out_ PHANDLE NewTokenHandle);

#pragma endregion

#pragma region Registry

NTSYSAPI
NTSTATUS
NTAPI
NtOpenKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes);

NTSYSAPI
NTSTATUS
NTAPI
NtCreateKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Reserved_ ULONG TitleIndex,
    _In_opt_ PUNICODE_STRING Class,
    _In_ ULONG CreateOptions,
    _Out_opt_ PULONG Disposition);

_When_(Length == 0,
       _Post_satisfies_(return < 0)
)
_When_(Length > 0,
       _Post_satisfies_(return <= 0)
)
_Success_(return == STATUS_SUCCESS)
_On_failure_(_When_(return == STATUS_BUFFER_OVERFLOW || return == STATUS_BUFFER_TOO_SMALL,
                    _Post_satisfies_(*ResultLength > Length))
)
NTSYSAPI
NTSTATUS
NTAPI
NtQueryValueKey(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _In_ KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    _Out_writes_bytes_to_(Length, *ResultLength) PVOID KeyValueInformation,
    _In_ ULONG Length,
    _Out_ PULONG ResultLength);

NTSYSAPI
NTSTATUS
NTAPI
NtSetValueKey(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _In_opt_ ULONG TitleIndex,
    _In_ ULONG Type,
    _In_reads_bytes_(DataSize) PVOID Data,
    _In_ ULONG DataSize);

NTSYSAPI
NTSTATUS
NTAPI
NtDeleteKey(
    _In_ HANDLE KeyHandle);

#pragma endregion

#pragma region Driver

NTSYSAPI
NTSTATUS
NTAPI
NtLoadDriver(
    _In_ PUNICODE_STRING DriverServiceName);

NTSYSAPI
NTSTATUS
NTAPI
NtUnloadDriver(
    _In_ PUNICODE_STRING DriverServiceName);

#pragma endregion

#pragma region Event

NTSYSAPI
NTSTATUS
NTAPI
NtCreateEvent(
   _Out_ PHANDLE EventHandle,
   _In_ ACCESS_MASK DesiredAccess,
   _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
   _In_ EVENT_TYPE EventType,
   _In_ BOOLEAN InitialState);

NTSYSAPI
NTSTATUS
NTAPI
NtSetEvent(
    _In_ HANDLE EventHandle,
    _Out_opt_ PLONG PreviousState);

#pragma endregion

#pragma region Wow64

/* Caution: Exported from Wow Ntdll.dll only!!! */

_Must_inspect_result_
__drv_allocatesMem(Mem)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtWow64AllocateVirtualMemory64(
    _In_ HANDLE ProcessHandle,
    _Inout_ _At_(*BaseAddress,
                 _Readable_bytes_(*RegionSize)
                 _Writable_bytes_(*RegionSize)
                 _Post_readable_byte_size_(*RegionSize)) PULONGLONG BaseAddress,
    _In_ ULONGLONG ZeroBits,
    _Inout_ PULONGLONG RegionSize,
    _In_ ULONG AllocationType,
    _In_ ULONG Protect);

NTSYSAPI
NTSTATUS
NTAPI
NtWow64QueryInformationProcess64(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _Out_writes_bytes_to_(ProcessInformationLength, *ReturnLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Out_opt_ PULONG ReturnLength);

NTSYSAPI
NTSTATUS
NTAPI
NtWow64ReadVirtualMemory64(
    _In_ HANDLE ProcessHandle,
    _In_ ULONGLONG BaseAddress,
    _Out_ PVOID Buffer,
    _In_ ULONGLONG NumberOfBytesToRead,
    _Out_opt_ PULONGLONG NumberOfBytesRead);

NTSYSAPI
NTSTATUS
NTAPI
NtWow64WriteVirtualMemory64(
    _In_ HANDLE ProcessHandle,
    _In_ ULONGLONG BaseAddress,
    _In_ PVOID Buffer,
    _In_ ULONGLONG NumberOfBytesToWrite,
    _Out_opt_ ULONGLONG NumberOfBytesWritten);

#pragma endregion

NTSYSAPI
NTSTATUS
NTAPI
NtQuerySystemInformation(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _Out_writes_bytes_to_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength);

NTSYSAPI
NTSTATUS
NTAPI
NtQueryPerformanceCounter(
    _Out_ PLARGE_INTEGER PerformanceCounter,
    _Out_opt_ PLARGE_INTEGER PerformanceFrequency);

NTSYSAPI
NTSTATUS
NTAPI
NtQueryInformationAtom(
    _In_ RTL_ATOM Atom,
    _In_ ATOM_INFORMATION_CLASS AtomInformationClass,
    _Out_writes_bytes_to_(AtomInformationLength, *ReturnLength) PVOID AtomInformation,
    _In_ ULONG AtomInformationLength,
    _Out_opt_ PULONG ReturnLength);

#pragma endregion

EXTERN_C_END
