#pragma once

#include "../MakeLifeEasier.h"

#include <wincrypt.h>

EXTERN_C_START

// Return FALSE to stop enumeration
typedef
_Function_class_(CRYPT_CERTENUMPROC_FN)
LOGICAL
CALLBACK
CRYPT_CERTENUMPROC_FN(
    _In_ PCCERT_CONTEXT CertContext,
    _In_opt_ PVOID Context);
typedef CRYPT_CERTENUMPROC_FN *PCRYPT_CERTENUMPROC_FN;

MLE_API
W32ERROR
NTAPI
Crypt_EnumBlobCertificates(
    _In_reads_bytes_(BlobSize) PBYTE BlobData,
    _In_ ULONG BlobSize,
    _In_ __callback PCRYPT_CERTENUMPROC_FN CertEnumProc,
    _In_opt_ PVOID Context);

EXTERN_C_END
