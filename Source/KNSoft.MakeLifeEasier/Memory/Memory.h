#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

#pragma region Heap

MLE_API
_Must_inspect_result_
__drv_allocatesMem(Mem)
NTSTATUS
NTAPI
Mem_Alloc(
    _Out_ _At_(*BaseAddress,
               _Readable_bytes_(Size)
               _Writable_bytes_(Size)
               _Post_readable_byte_size_(Size)) PVOID* BaseAddress,
    _In_ SIZE_T Size);

MLE_API
LOGICAL
NTAPI
Mem_Free(
    __drv_freesMem(Mem) _Frees_ptr_ PVOID BaseAddress);

#define Mem_AllocPtr(p) Mem_Alloc(&(p), sizeof(*(p)))

#pragma endregion

#pragma region Page

MLE_API
_Must_inspect_result_
__drv_allocatesMem(Mem)
NTSTATUS
NTAPI
Mem_AllocPage(
    _Out_ _At_(*BaseAddress,
               _Readable_bytes_(Size)
               _Writable_bytes_(Size)
               _Post_readable_byte_size_(Size)) PVOID* BaseAddress,
    _In_ SIZE_T Size,
    _In_ ULONG Protect);

MLE_API
NTSTATUS
NTAPI
Mem_ProtectPage(
    _In_ PVOID BaseAddress,
    _In_ SIZE_T Size,
    _In_ ULONG Protect,
    _Out_ PULONG OldProtect);

MLE_API
NTSTATUS
NTAPI
Mem_FreePage(
    __drv_freesMem(Mem) _Frees_ptr_ _In_ PVOID BaseAddress);

#pragma endregion

/// <summary>
/// Combines groups of structures into a new allocated buffer
/// </summary>
/// <param name="GroupCount">Number of groups</param>
/// <param name="Size">Size of single structure in bytes</param>
/// <param name="...">pst1, u1, pst2, u2, ...</param>
/// <returns>New allocated buffer that combined groups of structures input, should be freed by calling "Mem_Free"</returns>
MLE_API
_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
__drv_allocatesMem(Mem)
PVOID
WINAPIV
Mem_CombineStructEx(
    _In_ ULONG GroupCount,
    _In_ ULONG Size,
    ...);

#define Mem_CombineStruct(GroupCount, Type, ...) (Type*)Mem_CombineStructEx(GroupCount, sizeof(Type), __VA_ARGS__)

EXTERN_C_END
