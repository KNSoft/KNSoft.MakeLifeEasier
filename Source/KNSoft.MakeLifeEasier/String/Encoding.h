#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

FORCEINLINE
ULONG
Str_UnicodeToAnsi(
    _Out_writes_(BufferCount) PSTR Buffer,
    _In_ ULONG BufferCount,
    _In_ PCWSTR UnicodeString)
{
    ULONG BufferSize, TranslatedBytes, iCch;

    BufferSize = BufferCount * sizeof(Buffer[0]);
    TranslatedBytes = 0;
    RtlUnicodeToMultiByteN(Buffer, BufferSize, &TranslatedBytes, UnicodeString, (ULONG)Str_SizeW(UnicodeString));
    if (TranslatedBytes == 0 || TranslatedBytes >= BufferSize)
    {
        return 0;
    }

    iCch = TranslatedBytes / sizeof(CHAR);
    Buffer[iCch] = ANSI_NULL;
    return iCch;
}

FORCEINLINE
ULONG
Str_AnsiToUnicode(
    _Out_writes_(BufferCount) PWSTR Buffer,
    _In_ ULONG BufferCount,
    _In_ PCSTR AnsiString)
{
    ULONG BufferSize, TranslatedBytes, iCch;

    BufferSize = BufferCount * sizeof(Buffer[0]);
    TranslatedBytes = 0;
    RtlMultiByteToUnicodeN(Buffer, BufferSize, &TranslatedBytes, AnsiString, (ULONG)Str_SizeA(AnsiString));
    if (TranslatedBytes == 0 || TranslatedBytes >= BufferSize)
    {
        return 0;
    }

    iCch = TranslatedBytes / sizeof(WCHAR);
    Buffer[iCch] = UNICODE_NULL;
    return iCch;
}

FORCEINLINE
ULONG
Str_UnicodeToUtf8(
    _Out_writes_opt_(BufferCount) PSTR Buffer,
    _In_opt_ ULONG BufferCount,
    _In_ PCWSTR UnicodeString)
{
    NTSTATUS Status;
    ULONG BufferSize, TranslatedBytes, iCch;

    BufferSize = BufferCount * sizeof(Buffer[0]);
    if (Buffer == NULL)
    {
        Status = RtlUnicodeToUTF8N(NULL, 0, &TranslatedBytes, UnicodeString, (ULONG)Str_SizeW(UnicodeString));
        return NT_SUCCESS(Status) ? TranslatedBytes + sizeof(ANSI_NULL) : 0;
    }

    Status = RtlUnicodeToUTF8N(Buffer, BufferSize, &TranslatedBytes, UnicodeString, (ULONG)Str_SizeW(UnicodeString));
    if (!NT_SUCCESS(Status) || TranslatedBytes == 0 || TranslatedBytes >= BufferSize)
    {
        return 0;
    }

    iCch = TranslatedBytes / sizeof(CHAR);
    Buffer[iCch] = ANSI_NULL;
    return iCch;
}

FORCEINLINE
ULONG
Str_Utf8ToUnicode(
    _Out_writes_opt_(BufferCount) PWSTR Buffer,
    _In_opt_ ULONG BufferCount,
    _In_ PCSTR Utf8String)
{
    NTSTATUS Status;
    ULONG BufferSize, TranslatedBytes, iCch;

    BufferSize = BufferCount * sizeof(Buffer[0]);
    if (Buffer == NULL)
    {
        Status = RtlUTF8ToUnicodeN(NULL, 0, &TranslatedBytes, Utf8String, (ULONG)Str_SizeA(Utf8String));
        return NT_SUCCESS(Status) ? TranslatedBytes + sizeof(UNICODE_NULL) : 0;
    }

    Status = RtlUTF8ToUnicodeN(Buffer, BufferSize, &TranslatedBytes, Utf8String, (ULONG)Str_SizeA(Utf8String));
    if (!NT_SUCCESS(Status) || TranslatedBytes == 0 || TranslatedBytes >= BufferSize)
    {
        return 0;
    }

    iCch = TranslatedBytes / sizeof(WCHAR);
    Buffer[iCch] = UNICODE_NULL;
    return iCch;
}

#define Str_W2A(ADest, WSrc) Str_UnicodeToAnsi(ADest, ARRAYSIZE(ADest), WSrc)
#define Str_A2W(WDest, ASrc) Str_AnsiToUnicode(WDest, ARRAYSIZE(WDest), ASrc)
#define Str_W2U(UDest, WSrc) Str_UnicodeToUtf8(UDest, ARRAYSIZE(UDest), WSrc)
#define Str_U2W(WDest, USrc) Str_Utf8ToUnicode(WDest, ARRAYSIZE(WDest), USrc)

MLE_API
extern
BYTE Str_Utf8_BOM[3];

EXTERN_C_END
