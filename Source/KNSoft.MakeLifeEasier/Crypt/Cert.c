#include "../MakeLifeEasier.inl"

W32ERROR
NTAPI
Crypt_EnumBlobCertificates(
    _In_reads_bytes_(BlobSize) PBYTE BlobData,
    _In_ ULONG BlobSize,
    _In_ __callback PCRYPT_CERTENUMPROC_FN CertEnumProc,
    _In_opt_ PVOID Context)
{
    CRYPT_DATA_BLOB Blob = { BlobSize, BlobData };
    HCERTSTORE CertStore;
    PCCERT_CONTEXT CertContext;

    CertStore = CertOpenStore(CERT_STORE_PROV_PKCS7,
                              X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                              (HCRYPTPROV_LEGACY)NULL,
                              0,
                              &Blob);
    if (!CertStore)
    {
        return Err_GetLastError();
    }

    CertContext = NULL;
    while ((CertContext = CertEnumCertificatesInStore(CertStore, CertContext)) != NULL)
    {
        if (!CertEnumProc(CertContext, Context))
        {
            CertFreeCertificateContext(CertContext);
            break;
        }
    };

    CertCloseStore(CertStore, 0);
    return ERROR_SUCCESS;
}
