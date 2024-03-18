#pragma once

#include "../NT/MinDef.h"

NTSTATUS NTAPI Mem_HeapAlloc(
    _Out_ _At_(*Address,
               _Readable_bytes_(Size)
               _Writable_bytes_(Size)
               _Post_readable_byte_size_(Size)) PVOID* Address,
    _In_ SIZE_T Size);

BOOL NTAPI Mem_HeapFree(
    __drv_freesMem(Mem) _Frees_ptr_ PVOID Address);
