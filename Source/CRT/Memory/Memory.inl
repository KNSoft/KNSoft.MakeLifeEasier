#pragma once

#include "../../Include/NTAssassin/Extension.h"
#include "../../Include/NTAssassin/API/Ntdll.h"

_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
__forceinline
void* HeapMemAllocate(_In_ size_t Size, _In_opt_ ULONG Flags)
{
    return RtlAllocateHeap(CURRENT_PROCESS_HEAP, Flags, Size);
}

_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
__forceinline
void* HeapMemReAllocate(_Frees_ptr_opt_ void* Block, size_t Size, _In_opt_ ULONG Flags)
{
    return RtlReAllocateHeap(CURRENT_PROCESS_HEAP, Flags, Block, Size);
}

__forceinline
void HeapMemFree(_Frees_ptr_opt_ void* Block)
{
    RtlFreeHeap(CURRENT_PROCESS_HEAP, 0, Block);
}

_Check_return_
_CRT_HYBRIDPATCHABLE
__forceinline
size_t HeapMemSize(_Pre_notnull_ void* _Block)
{
    return RtlSizeHeap(CURRENT_PROCESS_HEAP, 0, _Block);
}
