#include "AbeDecrypt.h"

/* user-mode bcrypt.h does not define it (the KM header does); CNG
   ChaCha20-Poly1305 requires this chaining mode. The algorithm itself
   (BCRYPT_CHACHA20_POLY1305_ALGORITHM) is available since Windows 10 */
#ifndef BCRYPT_CHAIN_MODE_NIST
#define BCRYPT_CHAIN_MODE_NIST L"ChainingModeNIST"
#endif

/*** AEAD open (no AAD) ***/

static NTSTATUS
AbeAeadOpen(
    _In_ PCWSTR Algorithm,
    _In_ PCWSTR ChainingMode,
    _In_reads_bytes_(32) const BYTE* Key,
    _In_reads_bytes_(12) const BYTE* Nonce,
    _In_reads_bytes_(Length) const BYTE* CipherText,
    _In_ DWORD Length,
    _In_reads_bytes_(16) const BYTE* Tag,
    _Out_writes_bytes_(Length) PBYTE Plain)
{
    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO Auth;
    BCRYPT_ALG_HANDLE Alg = NULL;
    BCRYPT_KEY_HANDLE Cipher = NULL;
    PBYTE Object = NULL;
    ULONG ObjLen, Done, Result;
    NTSTATUS Status;

    Status = BCryptOpenAlgorithmProvider(&Alg, Algorithm, NULL, 0);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    Status = BCryptSetProperty(Alg,
                               BCRYPT_CHAINING_MODE,
                               (PUCHAR)ChainingMode,
                               (ULONG)(Str_SizeW(ChainingMode) + sizeof(WCHAR)),
                               0);
    if (!NT_SUCCESS(Status))
    {
        goto _Exit;
    }
    Status = BCryptGetProperty(Alg,
                               BCRYPT_OBJECT_LENGTH,
                               (PUCHAR)&ObjLen,
                               sizeof(ObjLen),
                               &Done,
                               0);
    if (!NT_SUCCESS(Status))
    {
        goto _Exit;
    }
    Object = Mem_Alloc(ObjLen);
    if (Object == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto _Exit;
    }
    Status = BCryptGenerateSymmetricKey(Alg, &Cipher, Object, ObjLen, (PUCHAR)Key, 32, 0);
    if (NT_SUCCESS(Status))
    {
        BCRYPT_INIT_AUTH_MODE_INFO(Auth);
        Auth.pbNonce = (PUCHAR)Nonce;
        Auth.cbNonce = 12;
        Auth.pbTag = (PUCHAR)Tag;
        Auth.cbTag = 16;
        Status = BCryptDecrypt(Cipher,
                               (PUCHAR)CipherText,
                               Length,
                               &Auth,
                               NULL,
                               0,
                               Plain,
                               Length,
                               &Result,
                               0);
    }

_Exit:
    if (Cipher != NULL)
    {
        BCryptDestroyKey(Cipher);
    }
    if (Object != NULL)
    {
        RtlSecureZeroMemory(Object, ObjLen);
        Mem_Free(Object);
    }
    BCryptCloseAlgorithmProvider(Alg, 0);
    return Status;
}

NTSTATUS
AbeAesGcmOpen(
    _In_reads_bytes_(32) const BYTE* Key,
    _In_reads_bytes_(12) const BYTE* Nonce,
    _In_reads_bytes_(Length) const BYTE* CipherText,
    _In_ DWORD Length,
    _In_reads_bytes_(16) const BYTE* Tag,
    _Out_writes_bytes_(Length) PBYTE Plain)
{
    return AbeAeadOpen(BCRYPT_AES_ALGORITHM,
                       BCRYPT_CHAIN_MODE_GCM,
                       Key,
                       Nonce,
                       CipherText,
                       Length,
                       Tag,
                       Plain);
}

NTSTATUS
AbeChaChaPoly1305Open(
    _In_reads_bytes_(32) const BYTE* Key,
    _In_reads_bytes_(12) const BYTE* Nonce,
    _In_reads_bytes_(Length) const BYTE* CipherText,
    _In_ DWORD Length,
    _In_reads_bytes_(16) const BYTE* Tag,
    _Out_writes_bytes_(Length) PBYTE Plain)
{
    return AbeAeadOpen(BCRYPT_CHACHA20_POLY1305_ALGORITHM,
                       BCRYPT_CHAIN_MODE_NIST,
                       Key,
                       Nonce,
                       CipherText,
                       Length,
                       Tag,
                       Plain);
}

NTSTATUS
AbeGcmDecrypt(
    _In_reads_bytes_(32) const BYTE* Key,
    _In_reads_bytes_(Length) const BYTE* Value,
    _In_ DWORD Length,
    _Out_writes_bytes_(Length) PBYTE Plain)
{
    if (Length < 3 + 12 + 16)
    {
        return STATUS_DATA_ERROR;
    }
    return AbeAesGcmOpen(Key,
                         Value + 3,
                         Value + 15,
                         Length - 3 - 12 - 16,
                         Value + Length - 16,
                         Plain);
}

/*** Local State os_crypt blobs ***/

_Success_(return != FALSE)
BOOL
AbeReadOsCryptBlob(
    _In_ PCWSTR UserDataDir,
    _In_ PCWSTR Field,
    _Out_writes_bytes_(BlobSize) PBYTE Blob,
    _In_ ULONG BlobSize,
    _Inout_ PULONG BlobLength)
{
    IJsonValue* Root = NULL;
    IJsonObject* RootObject = NULL, * OsCrypt = NULL;
    HSTRING Value = NULL;
    HSTRING_HEADER KeyHeader, FieldHeader;
    HSTRING Key, FieldStr;
    WCHAR LocalState[MAX_PATH];
    PCWSTR Wide;
    BOOL Ok = FALSE;

    Str_PrintfExW(LocalState, MAX_PATH, L"%ls\\Local State", UserDataDir);
    if (FAILED(Data_JsonParseUtf8File(LocalState, ABE_LOCAL_STATE_MAX, &Root)) ||
        FAILED(Root->lpVtbl->GetObject(Root, &RootObject)) ||
        FAILED(_Inline_WindowsCreateStringReference(L"os_crypt",
                                                    ARRAYSIZE(L"os_crypt") - 1,
                                                    &KeyHeader,
                                                    &Key)) ||
        FAILED(RootObject->lpVtbl->GetNamedObject(RootObject, Key, &OsCrypt)) ||
        FAILED(_Inline_WindowsCreateStringReference(Field,
                                                    (ULONG)(Str_SizeW(Field) / sizeof(WCHAR)),
                                                    &FieldHeader,
                                                    &FieldStr)) ||
        FAILED(OsCrypt->lpVtbl->GetNamedString(OsCrypt, FieldStr, &Value)))
    {
        goto Cleanup;
    }
    Wide = _Inline_WindowsGetStringRawBuffer(Value, NULL);
    Ok = CryptStringToBinaryW(Wide,
                              0,
                              CRYPT_STRING_BASE64,
                              Blob,
                              BlobLength,
                              NULL,
                              NULL);

Cleanup:
    if (Value != NULL)
    {
        _Inline_WindowsDeleteString(Value);
    }
    if (OsCrypt != NULL)
    {
        OsCrypt->lpVtbl->Release(OsCrypt);
    }
    if (RootObject != NULL)
    {
        RootObject->lpVtbl->Release(RootObject);
    }
    if (Root != NULL)
    {
        Root->lpVtbl->Release(Root);
    }
    return Ok;
}

_Success_(return != FALSE)
BOOL
AbeGetV10Key(
    _In_ const NET_BROWSER_INFO* Browser,
    _Out_writes_bytes_(ABE_KEY_SIZE) PBYTE Key)
{
    static BYTE Blob[2048];
    DATA_BLOB In, Out = { 0 };          /* Out is freed on failure paths */
    ULONG BlobLength = sizeof(Blob);    /* in/out capacity */
    BOOL Ok;

    if (!AbeReadOsCryptBlob(Browser->UserDataDir,
                            L"encrypted_key",
                            Blob,
                            sizeof(Blob),
                            &BlobLength) ||
        BlobLength <= 5 || memcmp(Blob, "DPAPI", 5) != 0)
    {
        return FALSE;
    }
    In.pbData = Blob + 5;
    In.cbData = BlobLength - 5;
    Ok = CryptUnprotectData(&In, NULL, NULL, NULL, NULL, 0, &Out);
    if (Ok)
    {
        Ok = Out.cbData == ABE_KEY_SIZE;
        if (Ok)
        {
            RtlCopyMemory(Key, Out.pbData, ABE_KEY_SIZE);
        }
        RtlSecureZeroMemory(Out.pbData, Out.cbData);
    }
    LocalFree(Out.pbData);
    return Ok;
}
