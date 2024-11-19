#pragma once

#include <KNSoft/NDK/NDK.h>

/*
 * If failed, the caller could use
 * STATUS_NO_MEMORY, ERROR_OUTOFMEMORY, ERROR_NOT_ENOUGH_MEMORY, E_OUTOFMEMORY, ... as error code
 */
FORCEINLINE
_Success_(return != NULL)
_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
__drv_allocatesMem(Mem)
DECLSPEC_ALLOCATOR
DECLSPEC_NOALIAS
DECLSPEC_RESTRICT
PVOID
Mem_Alloc(
    _In_ SIZE_T Size)
{
    return RtlAllocateHeap(NtGetProcessHeap(), 0, Size);
}

FORCEINLINE
_Success_(return != FALSE)
LOGICAL
Mem_Free(
    __drv_freesMem(Mem) _Frees_ptr_ _Post_invalid_ PVOID BaseAddress)
{
    return RtlFreeHeap(NtGetProcessHeap(), 0, BaseAddress);
}
