#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

#pragma region Heap

#define Mem_AllocPtr(p) ((LOGICAL)((p = (typeof(p))Mem_Alloc(sizeof(*(p)))) != NULL))

#pragma endregion

#pragma region Page

FORCEINLINE
_Must_inspect_result_
__drv_allocatesMem(Mem)
NTSTATUS
Mem_AllocPage(
    _Out_ _At_(*BaseAddress, _Readable_bytes_(Size) _Writable_bytes_(Size) _Post_readable_byte_size_(Size)) PVOID* BaseAddress,
    _In_ SIZE_T Size,
    _In_ ULONG Protect)
{
    PVOID Base = NULL;
    NTSTATUS Status;

    Status = NtAllocateVirtualMemory(NtCurrentProcess(), &Base, 0, &Size, MEM_COMMIT | MEM_RESERVE, Protect);
    if (NT_SUCCESS(Status))
    {
        *BaseAddress = Base;
    }
    return Status;
}

FORCEINLINE
NTSTATUS
Mem_ProtectPage(
    _In_ PVOID BaseAddress,
    _In_ SIZE_T Size,
    _In_ ULONG Protect,
    _Out_ PULONG OldProtect)
{
    return NtProtectVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, Protect, OldProtect);
}

FORCEINLINE
NTSTATUS
Mem_FreePage(
    __drv_freesMem(Mem) _Frees_ptr_ _In_ PVOID BaseAddress)
{
    SIZE_T Size = 0;

    return NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
}

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
