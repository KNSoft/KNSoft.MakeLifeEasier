#include "NTAssassin.Lib.inl"

NTSTATUS NTAPI Mem_HeapAlloc(
    _Out_ _At_(*Address,
               _Readable_bytes_(Size)
               _Writable_bytes_(Size)
               _Post_readable_byte_size_(Size)) PVOID* Address,
    _In_ SIZE_T Size)
{
    PVOID Buffer;

    Buffer = RtlAllocateHeap(NtGetProcessHeap(), 0, Size);
    if (Buffer == NULL)
    {
        /*
         * RtlAllocateHeap may sets variable NTSTATUS, but here returns STATUS_NO_MEMORY only if failed,
         * as well as most of Windows NT API does
         */
        return STATUS_NO_MEMORY;
    }

    *Address = Buffer;
    return STATUS_SUCCESS;
}

BOOL NTAPI Mem_HeapFree(
    __drv_freesMem(Mem) _Frees_ptr_ PVOID Address)
{
    return RtlFreeHeap(NtGetProcessHeap(), 0, Address);
}
