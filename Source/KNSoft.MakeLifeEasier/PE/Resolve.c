#include "../MakeLifeEasier.inl"

_Success_(return != FALSE)
LOGICAL
NTAPI
PE_ResolveOnline(
    _Out_ PPE_STRUCT PEStruct,
    _In_reads_bytes_(Size) PVOID Image,
    _In_ ULONG Size)
{
    PIMAGE_NT_HEADERS pNtHeader = Add2Ptr(Image, ((PIMAGE_DOS_HEADER)Image)->e_lfanew);
    PIMAGE_FILE_HEADER pFileHeader = &pNtHeader->FileHeader;
    PIMAGE_OPTIONAL_HEADER pOptHeader = &pNtHeader->OptionalHeader;
    PIMAGE_SECTION_HEADER pSectionHeader = Add2Ptr(pOptHeader, pFileHeader->SizeOfOptionalHeader);

    if (pOptHeader->SizeOfImage > Size)
    {
        return FALSE;
    }
    PEStruct->Image = (PBYTE)Image;
    PEStruct->Size = pOptHeader->SizeOfImage;
    PEStruct->FileHeader = pFileHeader;
    PEStruct->Bits = IS_WIN64 ? 64 : 32;
    PEStruct->OptionalHeader = pOptHeader;
    PEStruct->SectionHeader = pSectionHeader;
    PEStruct->OfflineMap = FALSE;
    PEStruct->OverlayData = NULL;
    PEStruct->OverlayDataSize = 0;

    return TRUE;
}

_Success_(return != FALSE)
LOGICAL
NTAPI
PE_ResolveOffline(
    _Out_ PPE_STRUCT PEStruct,
    _In_reads_bytes_(Size) PVOID Buffer,
    _In_ ULONG Size)
{
    /* Boundary check */
    PVOID pEndOfMap = Add2Ptr(Buffer, Size);

    /* DOS Header */
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)Buffer;
    if (Size < sizeof(IMAGE_DOS_HEADER) ||
        pDosHeader->e_magic != IMAGE_DOS_SIGNATURE ||
        pDosHeader->e_lfanew < sizeof(IMAGE_DOS_HEADER) ||
        Size < pDosHeader->e_lfanew + RTL_SIZEOF_THROUGH_FIELD(IMAGE_NT_HEADERS, FileHeader))
    {
        return FALSE;
    }

    /* NT Headers */
    ULONG ulBits;
    WORD wOptHeaderSize;
    PIMAGE_NT_HEADERS pNtHeader = Add2Ptr(pDosHeader, pDosHeader->e_lfanew);
    if (Add2Ptr(pNtHeader, RTL_SIZEOF_THROUGH_FIELD(IMAGE_NT_HEADERS, OptionalHeader.Magic)) > pEndOfMap)
    {
        return FALSE;
    }
    if (pNtHeader->Signature != IMAGE_NT_SIGNATURE)
    {
        return FALSE;
    }
    PIMAGE_FILE_HEADER pFileHeader = &pNtHeader->FileHeader;
    PIMAGE_OPTIONAL_HEADER pOptHeader = &pNtHeader->OptionalHeader;
    if (pOptHeader->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        ulBits = 64;
        wOptHeaderSize = sizeof(IMAGE_OPTIONAL_HEADER64);
    } else if (pOptHeader->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        ulBits = 32;
        wOptHeaderSize = sizeof(IMAGE_OPTIONAL_HEADER32);
    } else if (pOptHeader->Magic == IMAGE_ROM_OPTIONAL_HDR_MAGIC)
    {
        ulBits = 0;
        wOptHeaderSize = sizeof(IMAGE_ROM_OPTIONAL_HEADER);
    } else
    {
        return FALSE;
    }
    if (pFileHeader->SizeOfOptionalHeader < wOptHeaderSize)
    {
        return FALSE;
    }

    /* Section Headers */
    PIMAGE_SECTION_HEADER pSecHeader = Add2Ptr(pOptHeader, pFileHeader->SizeOfOptionalHeader);
    if (Add2Ptr(pSecHeader, sizeof(IMAGE_SECTION_HEADER) * ((SIZE_T)pFileHeader->NumberOfSections + 1)) > pEndOfMap)
    {
        return FALSE;
    }

    /* Overlay */
    USHORT u;
    ULONG ulEndOfSec, ulFileSize = 0;
    PIMAGE_SECTION_HEADER pSection = pSecHeader;
    for (u = 0; u < pFileHeader->NumberOfSections; u++)
    {
        ulEndOfSec = pSection->PointerToRawData + pSection->SizeOfRawData;
        if (ulEndOfSec > Size)
        {
            return FALSE;
        } else if (ulEndOfSec > ulFileSize)
        {
            ulFileSize = ulEndOfSec;
        }
        pSection++;
    }

    /* Fill Structure */
    if (Size > ulFileSize)
    {
        PEStruct->OverlayData = Add2Ptr(Buffer, ulFileSize);
        PEStruct->OverlayDataSize = Size - ulFileSize;
    } else
    {
        PEStruct->OverlayData = NULL;
        PEStruct->OverlayDataSize = 0;
    }
    PEStruct->Image = Buffer;
    PEStruct->Size = Size;
    PEStruct->OfflineMap = TRUE;
    PEStruct->FileHeader = pFileHeader;
    PEStruct->Bits = ulBits;
    PEStruct->OptionalHeader = pOptHeader;
    PEStruct->SectionHeader = pSecHeader;

    return TRUE;
}

_Success_(return != NULL)
_Ret_maybenull_
PIMAGE_SECTION_HEADER
NTAPI
PE_GetSectionByRVA(
    _In_ PPE_STRUCT PEStruct,
    _In_ DWORD RVA)
{
    USHORT u, uSections;
    PIMAGE_SECTION_HEADER pSection;
    DWORD dwSecVA;

    uSections = PEStruct->FileHeader->NumberOfSections;
    pSection = PEStruct->SectionHeader;
    for (u = 0; u < uSections; u++)
    {
        dwSecVA = pSection->VirtualAddress;
        if (RVA >= dwSecVA && RVA < dwSecVA + pSection->Misc.VirtualSize)
        {
            return pSection;
        }
        pSection++;
    }

    return NULL;
}

_Success_(return != NULL)
_Ret_maybenull_
PIMAGE_SECTION_HEADER
NTAPI
PE_GetSectionByOffset(
    _In_ PPE_STRUCT PEStruct,
    _In_ DWORD Offset)
{
    USHORT u, uSections;
    PIMAGE_SECTION_HEADER pSection;
    DWORD dwSecOffset;

    uSections = PEStruct->FileHeader->NumberOfSections;
    pSection = PEStruct->SectionHeader;
    for (u = 0; u < uSections; u++)
    {
        dwSecOffset = pSection->PointerToRawData;
        if (Offset >= dwSecOffset && Offset < dwSecOffset + pSection->SizeOfRawData)
        {
            return pSection;
        }
        pSection++;
    }

    return NULL;
}

_Success_(return != NULL)
_Ret_maybenull_
PVOID
NTAPI
PE_RVA2Ptr(
    _In_ PPE_STRUCT PEStruct,
    _In_ DWORD RVA)
{
    ULONG uDelta;
    PIMAGE_SECTION_HEADER pSection;

    if (PEStruct->OfflineMap)
    {
        pSection = PE_GetSectionByRVA(PEStruct, RVA);
        if (pSection == NULL)
        {
            return NULL;
        }
        uDelta = RVA - pSection->VirtualAddress + pSection->PointerToRawData;
    } else
    {
        uDelta = RVA;
    }
    return uDelta < PEStruct->Size ? Add2Ptr(PEStruct->Image, uDelta) : NULL;
}

_Success_(return != FALSE)
LOGICAL
NTAPI
PE_Ptr2RVA(
    _In_ PPE_STRUCT PEStruct,
    _In_ PVOID Ptr,
    _Out_ PDWORD RVA)
{
    DWORD Delta = PtrOffset(PEStruct->Image, Ptr);
    PIMAGE_SECTION_HEADER pSection;

    if (PEStruct->OfflineMap)
    {
        pSection = PE_GetSectionByOffset(PEStruct, Delta);
        if (pSection == NULL)
        {
            return FALSE;
        }
        Delta = Delta - pSection->PointerToRawData + pSection->VirtualAddress;
    }
    *RVA = Delta;
    return TRUE;
}

_Success_(return != FALSE)
LOGICAL
NTAPI
PE_Ptr2Offset(
    _In_ PPE_STRUCT PEStruct,
    _In_ PVOID Ptr,
    _Out_ PDWORD Offset)
{
    DWORD Delta = PtrOffset(PEStruct->Image, Ptr);
    PIMAGE_SECTION_HEADER pSection;

    if (!PEStruct->OfflineMap)
    {
        pSection = PE_GetSectionByRVA(PEStruct, Delta);
        if (pSection == NULL)
        {
            return FALSE;
        }
        Delta = Delta - pSection->VirtualAddress + pSection->PointerToRawData;
    }
    *Offset = Delta;
    return TRUE;
}

_Success_(return != FALSE)
LOGICAL
NTAPI
PE_GetExportedName(
    _In_ PPE_STRUCT PEStruct,
    _In_ PVOID Function,
    _Out_ PZPCSTR Name)
{
    DWORD dwFuncRVA;
    
    /* Get address of export table */
    if (!PE_Ptr2RVA(PEStruct, Function, &dwFuncRVA))
    {
        return FALSE;
    }
    PIMAGE_DATA_DIRECTORY pExportDir = PE_GetDataDirectory(PEStruct, IMAGE_DIRECTORY_ENTRY_EXPORT);
    if (!pExportDir ||
        !PE_ProbeForRead(PEStruct, pExportDir, sizeof(IMAGE_DATA_DIRECTORY)))
    {
        return FALSE;
    }
    PIMAGE_EXPORT_DIRECTORY pExportTable = PE_RVA2Ptr(PEStruct, pExportDir->VirtualAddress);
    if (!pExportTable ||
        !PE_ProbeForRead(PEStruct, pExportTable, sizeof(IMAGE_EXPORT_DIRECTORY)))
    {
        return FALSE;
    }
    PDWORD padwFuncRVAs = PE_RVA2Ptr(PEStruct, pExportTable->AddressOfFunctions);
    if (!padwFuncRVAs ||
        !PE_ProbeForRead(PEStruct, padwFuncRVAs, pExportTable->NumberOfFunctions * sizeof(*padwFuncRVAs)))
    {
        return FALSE;
    }

    /* Find exported functions */
    UINT uFuncIndex;
    for (uFuncIndex = 0; uFuncIndex < pExportTable->NumberOfFunctions; uFuncIndex++, padwFuncRVAs++)
    {
        if (*padwFuncRVAs == dwFuncRVA)
        {
            break;
        }
    }
    if (uFuncIndex >= pExportTable->NumberOfFunctions)
    {
        return FALSE;
    }

    /* Get name of function */
    PCSTR pszName = NULL;
    PWORD pawNameOrds = PE_RVA2Ptr(PEStruct, pExportTable->AddressOfNameOrdinals);
    if (!pawNameOrds ||
        !PE_ProbeForRead(PEStruct, pawNameOrds, pExportTable->NumberOfNames * sizeof(*pawNameOrds)))
    {
        return FALSE;
    }
    UINT uNameIndex;
    for (uNameIndex = 0; uNameIndex < pExportTable->NumberOfNames; uNameIndex++, pawNameOrds++)
    {
        if (*pawNameOrds == (WORD)uFuncIndex)
        {
            break;
        }
    }
    if (uNameIndex >= pExportTable->NumberOfNames)
    {
        return FALSE;
    }
    PDWORD padwNameRVAs = PE_RVA2Ptr(PEStruct, pExportTable->AddressOfNames);
    if (padwNameRVAs &&
        PE_ProbeForRead(PEStruct, &padwNameRVAs[uNameIndex], sizeof(*padwNameRVAs)))
    {
        pszName = PE_RVA2Ptr(PEStruct, padwNameRVAs[uNameIndex]);
    }
    *Name = pszName ? pszName : (PCSTR)(DWORD_PTR)(uFuncIndex + (DWORD_PTR)pExportTable->Base);

    return TRUE;
}

_Success_(return != NULL)
_Ret_maybenull_
LPWIN_CERTIFICATE
PE_GetCertificate(
    _In_ PPE_STRUCT PEStruct)
{
    LPWIN_CERTIFICATE pCertificate;
    PIMAGE_DATA_DIRECTORY pCertificateDir;

    if (!PEStruct->OfflineMap)
    {
        return NULL;
    }

    pCertificateDir = PE_GetDataDirectory(PEStruct, IMAGE_DIRECTORY_ENTRY_SECURITY);
    if (pCertificateDir == NULL ||
        !PE_ProbeForRead(PEStruct, pCertificateDir, sizeof(*pCertificateDir)) ||
        pCertificateDir->VirtualAddress == 0 ||
        pCertificateDir->Size == 0)
    {
        return NULL;
    }

    pCertificate = Add2Ptr(PEStruct->Image, pCertificateDir->VirtualAddress);
    if (!PE_ProbeForRead(PEStruct, pCertificate, pCertificateDir->Size))
    {
        return NULL;
    }

    return pCertificate;
}
