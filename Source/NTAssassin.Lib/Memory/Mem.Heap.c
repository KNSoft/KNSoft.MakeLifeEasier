#include "NTAssassin.Lib.inl"

NTSTATUS NTAPI Mem_HeapAlloc(
    _Out_ _At_(*Address,
               _Readable_bytes_(Size)
               _Writable_bytes_(Size)
               _Post_readable_byte_size_(Size)) PVOID* Address,
    _In_ SIZE_T Size)
{
    return NTA_Alloc(Address, Size);
}

BOOL NTAPI Mem_HeapFree(
    __drv_freesMem(Mem) _Frees_ptr_ PVOID Address)
{
    return NTA_Free(Address);
}
