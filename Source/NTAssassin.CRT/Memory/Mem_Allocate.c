#include "../CRT.inl"
#include "Memory.inl"

_Check_return_
_Ret_maybenull_
_Post_writable_byte_size_(_Size)
_ACRTIMP
_CRTALLOCATOR
_CRT_JIT_INTRINSIC
_CRTRESTRICT
_CRT_HYBRIDPATCHABLE
void* __cdecl malloc(_In_ _CRT_GUARDOVERFLOW size_t _Size)
{
    return HeapMemAllocate(_Size, 0);
}

_Check_return_
_Ret_maybenull_
_Post_writable_byte_size_(_Count * _Size)
_CRT_JIT_INTRINSIC
_CRTALLOCATOR
_CRTRESTRICT
_CRT_HYBRIDPATCHABLE
void* __cdecl calloc(_In_ _CRT_GUARDOVERFLOW size_t _Count, _In_ _CRT_GUARDOVERFLOW size_t _Size)
{
    return HeapMemAllocate(_Count * _Size, HEAP_ZERO_MEMORY);
}

_Success_(return != 0)
_Check_return_
_Ret_maybenull_
_Post_writable_byte_size_(_Size)
_CRTALLOCATOR
_CRTRESTRICT
_CRT_HYBRIDPATCHABLE
void* __cdecl realloc(_Pre_maybenull_ _Post_invalid_ void* _Block, _In_ _CRT_GUARDOVERFLOW size_t _Size)
{
    if (_Size == 0)
    {
        HeapMemFree(_Block);
        return NULL;
    }

    return HeapMemReAllocate(_Block, _Size, 0);
}

_Check_return_
_Ret_maybenull_
_Post_writable_byte_size_(_Size)
_CRTALLOCATOR
_CRT_HYBRIDPATCHABLE
void* __cdecl _expand(_Pre_notnull_ void* _Block, _In_ _CRT_GUARDOVERFLOW size_t _Size)
{
    return HeapMemReAllocate(_Block, _Size, HEAP_REALLOC_IN_PLACE_ONLY);
}

_CRT_HYBRIDPATCHABLE
void __cdecl free(_Pre_maybenull_ _Post_invalid_ void* _Block)
{
    HeapMemFree(_Block);
}

_Check_return_
_CRT_HYBRIDPATCHABLE
size_t __cdecl _msize(_Pre_notnull_ void* _Block)
{
    return HeapMemSize(_Block);
}
