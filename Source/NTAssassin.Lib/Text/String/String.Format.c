#include "NTAssassin.Lib.inl"

#include <stdio.h>

_Success_(
    return > 0 && return < BufferCount
)
ULONG WINAPIV String_CchVPrintfA(
    _Out_writes_opt_(BufferCount) _Always_(_Post_z_) PSTR CONST Buffer,
    _In_ SIZE_T CONST BufferCount,
    _In_z_ _Printf_format_string_ PCSTR Format,
    va_list ArgList)
{
    int i;

#pragma warning(disable: 4996)
    i = _vsnprintf(Buffer, BufferCount, Format, ArgList);
#pragma warning(default: 4996)
    if (i > 0)
    {
        if (Buffer != NULL && i == BufferCount)
        {
            Buffer[i - 1] = ANSI_NULL;
        }
        return i;
    } else if (i == 0)
    {
        return 0;
    }

#pragma warning(disable: 4996)
    i = _vsnprintf(NULL, 0, Format, ArgList);
#pragma warning(default: 4996)
    if (i > 0)
    {
        if (Buffer != NULL && (SIZE_T)i > BufferCount && BufferCount > 0)
        {
            Buffer[BufferCount - 1] = ANSI_NULL;
        }
        return i;
    }

    return 0;
}

_Success_(
    return > 0 && return < BufferCount
)
ULONG WINAPIV String_CchVPrintfW(
    _Out_writes_opt_(BufferCount) _Always_(_Post_z_) PWSTR CONST Buffer,
    _In_ SIZE_T CONST BufferCount,
    _In_z_ _Printf_format_string_ PCWSTR Format,
    va_list ArgList)
{
    int i;

#pragma warning(disable: 4996)
    i = _vsnwprintf(Buffer, BufferCount, Format, ArgList);
#pragma warning(default: 4996)
    if (i > 0)
    {
        if (Buffer != NULL && i == BufferCount)
        {
            Buffer[i - 1] = UNICODE_NULL;
        }
        return i;
    } else if (i == 0)
    {
        return 0;
    }

#pragma warning(disable: 4996)
    i = _vsnwprintf(NULL, 0, Format, ArgList);
#pragma warning(default: 4996)
    if (i > 0)
    {
        if (Buffer != NULL && (SIZE_T)i > BufferCount && BufferCount > 0)
        {
            Buffer[BufferCount - 1] = UNICODE_NULL;
        }
        return i;
    }

    return 0;
}

_Success_(
    return > 0 && return < BufferCount
)
ULONG String_CchPrintfA(
    _Out_writes_opt_(BufferCount) _Always_(_Post_z_) PSTR CONST Buffer,
    _In_ SIZE_T CONST BufferCount,
    _In_z_ _Printf_format_string_ PCSTR Format, ...)
{
    va_list argList;

    va_start(argList, Format);
    return String_CchVPrintfA(Buffer, BufferCount, Format, argList);
}

_Success_(
    return > 0 && return < BufferCount
)
ULONG String_CchPrintfW(
    _Out_writes_opt_(BufferCount) _Always_(_Post_z_) PWSTR CONST Buffer,
    _In_ SIZE_T CONST BufferCount,
    _In_z_ _Printf_format_string_ PCWSTR Format, ...)
{
    va_list argList;

    va_start(argList, Format);
    return String_CchVPrintfW(Buffer, BufferCount, Format, argList);
}
