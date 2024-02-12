#include "../Test.h"

#include <vcruntime_new.h>
#include <malloc.h>

constexpr size_t sInitSize1 = 0x2345;
constexpr size_t sInitSize2 = 0x1234;
constexpr size_t sInitSize3 = 0x5432;

BOOL Test_new_delete()
{
    void* pv;
    char* pc;

    /* Test new(std::nothrow)[] and delete[] */
    pv = new(std::nothrow) char[100];
    if (pv != NULL)
    {
        delete[] pv;
    } else
    {
        PrintF("new(std::nothrow) char[] failed with 0x%08lX\n", NtGetLastStatus());
        return FALSE;
    }

    /* Test new()[] and delete[] */
    pv = new char[100];
    if (pv != NULL)
    {
        delete[] pv;
    } else
    {
        PrintF("new char[] failed with 0x%08lX\n", NtGetLastStatus());
        return FALSE;
    }

    /* Test new() and delete */
    pc = new char;
    if (pc != NULL)
    {
        delete pc;
    } else
    {
        PrintF("new char failed with 0x%08lX\n", NtGetLastStatus());
        return FALSE;
    }

    return TRUE;
}

BOOL Test_Memory_Allocate()
{
    void* p;
    void* t;
    size_t s;

    p = malloc(sInitSize1);
    if (p == NULL)
    {
        PrintF("malloc failed\n");
        return FALSE;
    }

    s = _msize(p);
    if (s != sInitSize1)
    {
        PrintF("_msize returns 0x%p but expect 0x%p\n", (PVOID)s, (PVOID)sInitSize1);
        free(p);
        return FALSE;
    }

    t = _expand(p, sInitSize2);
    if (t != NULL && t != p)
    {
        PrintF("_expand returns a valid pointer 0x%p and differernt from original pointer 0x%p\n", t, p);
    }

    s = _msize(p);
    if (s != sInitSize2)
    {
        PrintF("_msize returns 0x%p but expect 0x%p\n", (PVOID)s, (PVOID)sInitSize2);
        free(p);
        return FALSE;
    }

    t = realloc(p, sInitSize3);
    if (t == NULL)
    {
        PrintF("realloc failed\n");
        free(p);
        return FALSE;
    }
    p = t;

    s = _msize(p);
    if (s != sInitSize3)
    {
        PrintF("_msize returns 0x%p but expect 0x%p\n", (PVOID)s, (PVOID)sInitSize3);
        free(p);
        return FALSE;
    }

    free(p);
    return TRUE;
}
