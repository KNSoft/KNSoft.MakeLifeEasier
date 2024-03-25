#pragma once

#include "../NDK/NDK.h"

#if _WINDLL
#ifdef _NTASSASSIN_BUILD_
#define NTA_API DECLSPEC_EXPORT
#else
#define NTA_API DECLSPEC_IMPORT
#endif
#else
#define NTA_API
#endif

#include "./Memory.h"
#include "./Text.h"
#include "./NT.h"
#include "./IO.h"
#include "./UnitTestFramework.h"

#pragma region Initialization

EXTERN_C_START

BOOL NTAPI NTA_Initialize();

EXTERN_C BOOL _NTA_Initialized;
EXTERN_C HANDLE _NTA_Heap;
EXTERN_C CPUID_INFO _NTA_CpuidInfo_00_00;
EXTERN_C CPUID_INFO _NTA_CpuidInfo_01_00;
EXTERN_C CPUID_INFO _NTA_CpuidInfo_07_00;
EXTERN_C CPUID_INFO _NTA_CpuidInfo_07_01;
EXTERN_C unsigned __int64 _NTA_XCR_XFeature;
EXTERN_C PPEB _NTA_PEB;
EXTERN_C USHORT _NTA_Process_WinntVersion;

EXTERN_C_END

#pragma endregion

#pragma region Common memory alloc/free

FORCEINLINE NTSTATUS NTAPI NTA_Alloc(
    _Out_ _At_(*Address,
               _Readable_bytes_(Size)
               _Writable_bytes_(Size)
               _Post_readable_byte_size_(Size)) PVOID* Address,
    _In_ SIZE_T Size)
{
    PVOID Buffer;

    Buffer = RtlAllocateHeap(_NTA_Heap, 0, Size);
    if (Buffer == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    *Address = Buffer;
    return STATUS_SUCCESS;
}

FORCEINLINE BOOL NTAPI NTA_Free(
    __drv_freesMem(Mem) _Frees_ptr_ PVOID Address)
{
    return RtlFreeHeap(_NTA_Heap, 0, Address);
}

#pragma endregion
