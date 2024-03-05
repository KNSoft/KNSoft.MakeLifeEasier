#include "../CRT.inl"
#include "Memory.inl"

#include <vcruntime_new.h>
#include <suppress.h>

#define _CRT_ENABLE_SELECTANY_NOTHROW

/*
 * This implementation like old MSVC which returned a null pointer on an allocation failure,
 * while new MSVC throws std::bad_alloc.
 * So C28196(__WARNING_RETURNING_BAD_RESULT) and C6387(__WARNING_INVALID_PARAM_VALUE_1) are dismissed for a while.
 * See also: https://learn.microsoft.com/en-us/cpp/build/reference/zc-throwingnew-assume-operator-new-throws
 */

namespace std
{
#ifdef _CRT_ENABLE_SELECTANY_NOTHROW
    __declspec(selectany) nothrow_t const nothrow;
#else
    nothrow_t const nothrow;
#endif
}

#pragma warning(push)
#pragma warning(disable: __WARNING_RETURNING_BAD_RESULT)
#pragma warning(disable: __WARNING_INVALID_PARAM_VALUE_1)

_NODISCARD
_Ret_notnull_
_Post_writable_byte_size_(_Size)
_VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t _Size)
{
    return HeapMemAllocate(_Size, 0);
}

_NODISCARD
_Ret_notnull_
_Post_writable_byte_size_(_Size)
_VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t _Size)
{
    return HeapMemAllocate(_Size, 0);
}

#pragma warning(pop)

_NODISCARD
_Ret_maybenull_
_Success_(return != NULL)
_Post_writable_byte_size_(_Size)
_VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t _Size, ::std::nothrow_t const&) noexcept
{
    return HeapMemAllocate(_Size, 0);
}

_NODISCARD
_Ret_maybenull_
_Success_(return != NULL)
_Post_writable_byte_size_(_Size)
_VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t _Size, ::std::nothrow_t const&) noexcept
{
    return HeapMemAllocate(_Size, 0);
}

void __CRTDECL operator delete(void* _Block) noexcept
{
    HeapMemFree(_Block);
}

void __CRTDECL operator delete(void* _Block, ::std::nothrow_t const&) noexcept
{
    HeapMemFree(_Block);
}

void __CRTDECL operator delete[](void* _Block) noexcept
{
    HeapMemFree(_Block);
}

void __CRTDECL operator delete[](void* _Block, ::std::nothrow_t const&) noexcept
{
    HeapMemFree(_Block);
}

void __CRTDECL operator delete(void* _Block, size_t _Size) noexcept
{
    HeapMemFree(_Block);
}

void __CRTDECL operator delete[](void* _Block, size_t _Size) noexcept
{
    HeapMemFree(_Block);
}
