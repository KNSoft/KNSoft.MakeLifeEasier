#include "../MakeLifeEasier.inl"

_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
__drv_allocatesMem(Mem)
PVOID
WINAPIV
Mem_CombineStructEx(
    _In_ ULONG GroupCount,
    _In_ ULONG Size,
    ...)
{
    va_list args;
    SIZE_T i, u, uStructCount = 0;
    PVOID pBuffer, pDst, pSrc;

    va_start(args, Size);
    for (i = 0; i < GroupCount; i++)
    {
        pSrc = va_arg(args, PVOID);
        u = va_arg(args, ULONG);
        if (pSrc)
        {
            uStructCount += u;
        }
    }
    pBuffer = Mem_Alloc(uStructCount * Size);
    if (pBuffer == NULL)
    {
        return NULL;
    }

    pDst = pBuffer;
    va_start(args, Size);
    for (i = 0; i < GroupCount; i++)
    {
        pSrc = va_arg(args, PVOID);
        uStructCount = va_arg(args, ULONG);
        if (pSrc)
        {
            memcpy(pDst, pSrc, uStructCount * Size);
            pDst = Add2Ptr(pDst, uStructCount * Size);
        }
    }
    va_end(args);

    return pBuffer;
}
