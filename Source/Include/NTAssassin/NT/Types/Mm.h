#pragma once

#include "../MinDef.h"

#define PAGE_SIZE 0x1000
#define MM_ALLOCATION_GRANULARITY 0x10000
#define MM_SHARED_USER_DATA_VA 0x7FFE0000

#if defined(_WIN64)
#define MM_HIGHEST_USER_ADDRESS ((PVOID)0x000007FFFFFEFFFFULL)
#else
#define MM_HIGHEST_USER_ADDRESS ((PVOID)0x7FFEFFFFUL)
#endif
#define MM_LOWEST_USER_ADDRESS ((PVOID)0x10000)

#if defined(_NTASSASSIN_NTDEF_EXTENSION_)

/* 
 * ASLR initialization constants, see:
 * "Image randomization." Microsoft Windows Internals
 * ntoskrnl.exe!MiInitializeRelocations
 */

#if defined(_WIN64)

/* [0x00007FF7FFFF0000 ... 0x00007FFFFFFF0000], 32G */

#define MI_ASLR_BITMAP_SIZE 0x10000
#define MI_ASLR_HIGHEST_SYSTEM_RANGE_ADDRESS ((PVOID)0x00007FFFFFFF0000ULL)

#else

/* [0x50000000 ... 0x78000000], 640M */

#define MI_ASLR_BITMAP_SIZE 0x500
#define MI_ASLR_HIGHEST_SYSTEM_RANGE_ADDRESS ((PVOID)0x78000000UL)

#endif

#endif /* defined(_NTASSASSIN_NTDEF_EXTENSION_) */

typedef enum _SECTION_INHERIT
{
    ViewShare = 1,
    ViewUnmap = 2
} SECTION_INHERIT;

typedef struct _MEMORY_RANGE_ENTRY
{
    PVOID VirtualAddress;
    SIZE_T NumberOfBytes;
} MEMORY_RANGE_ENTRY, *PMEMORY_RANGE_ENTRY;

typedef enum _MEMORY_INFORMATION_CLASS
{
    MemoryBasicInformation,
    MemoryWorkingSetList,
    MemorySectionName,
    MemoryBasicVlmInformation,
    MemoryWorkingSetExList
} MEMORY_INFORMATION_CLASS, *PMEMORY_INFORMATION_CLASS;
