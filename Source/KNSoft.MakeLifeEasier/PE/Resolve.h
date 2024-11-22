#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

typedef struct _PE_STRUCT
{
    PBYTE                           Image;
    ULONG                           Size;
    BOOL                            OfflineMap;
    PIMAGE_FILE_HEADER              FileHeader;
    ULONG                           Bits;
    union
    {
        PIMAGE_OPTIONAL_HEADER      OptionalHeader;
        PIMAGE_OPTIONAL_HEADER64    OptionalHeader64;   // Bits = 64
        PIMAGE_OPTIONAL_HEADER32    OptionalHeader32;   // Bits = 32
        PIMAGE_ROM_OPTIONAL_HEADER  OptionalHeaderRom;  // Bits = 0
    };
    PIMAGE_SECTION_HEADER           SectionHeader;
    PVOID                           OverlayData;
    ULONG                           OverlayDataSize;
} PE_STRUCT, *PPE_STRUCT;

FORCEINLINE
LOGICAL
PE_ProbeForRead(
    _In_ PPE_STRUCT PEStruct,
    _In_ PVOID Pointer,
    _In_ ULONG Size)
{
    return Pointer >= (PVOID)PEStruct->Image &&
        Add2Ptr(Pointer, Size) <= Add2Ptr(PEStruct->Image, + PEStruct->Size);
}

/// <summary>
/// Resolve a online PE image to PE_STRUCT structure
/// </summary>
/// <param name="PEStruct">Pointer to a PE_STRUCT structure</param>
/// <param name="Image">Pointer to the image</param>
/// <param name="Size">Readable size of Image parameter</param>
MLE_API
_Success_(return != FALSE)
LOGICAL
NTAPI
PE_ResolveOnline(
    _Out_ PPE_STRUCT PEStruct,
    _In_reads_bytes_(Size) PVOID Image,
    _In_ ULONG Size);

/// <summary>
/// Resolve a offline PE image to PE_STRUCT structure
/// </summary>
/// <param name="PEStruct">Pointer to a PE_STRUCT structure</param>
/// <param name="Buffer">Pointer to the buffer</param>
/// <param name="Size">Size of buffer</param>
MLE_API
_Success_(return != FALSE)
LOGICAL
NTAPI
PE_ResolveOffline(
    _Out_ PPE_STRUCT PEStruct,
    _In_reads_bytes_(Size) PVOID Buffer,
    _In_ ULONG Size);

#define PE_GetOptionalHeaderValue(pe, m) (\
    (pe)->Bits == 64 ? (pe)->OptionalHeader64->m : (\
        (pe)->Bits == 32 ? (pe)->OptionalHeader32->m : (\
            (pe)->Bits == 0 ? (pe)->OptionalHeaderRom->m :\
                (__fastfail(FAST_FAIL_INVALID_ARG), 0)\
            )\
         )\
    )

/// <summary>
/// Get data directory entry of PE image
/// </summary>
/// <param name="PEStruct">Pointer to the PE_STRUCT structure</param>
/// <param name="Index">IMAGE_DIRECTORY_ENTRY_XXX</param>
/// <returns>Pointer to the IMAGE_DATA_DIRECTORY structure, or NULL if failed</returns>
FORCEINLINE
_Success_(return != NULL)
_Ret_maybenull_
PIMAGE_DATA_DIRECTORY
PE_GetDataDirectory(
    _In_ PPE_STRUCT PEStruct,
    _In_ ULONG Index)
{
    if (PEStruct->Bits == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        return &PEStruct->OptionalHeader32->DataDirectory[Index];
    } else if (PEStruct->Bits == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        return &PEStruct->OptionalHeader64->DataDirectory[Index];
    } else
    {
        return NULL;
    }
}

/// <summary>
/// Get section pointer by RVA
/// </summary>
MLE_API
_Success_(return != NULL)
_Ret_maybenull_
PIMAGE_SECTION_HEADER
NTAPI
PE_GetSectionByRVA(
    _In_ PPE_STRUCT PEStruct,
    _In_ DWORD RVA);

/// <summary>
/// Get section pointer by Offset
/// </summary>
MLE_API
_Success_(return != NULL)
_Ret_maybenull_
PIMAGE_SECTION_HEADER
NTAPI
PE_GetSectionByOffset(
    _In_ PPE_STRUCT PEStruct,
    _In_ DWORD Offset);

/// <summary>
/// Convert RVA to pointer
/// </summary>
MLE_API
_Success_(return != NULL)
_Ret_maybenull_
PVOID
NTAPI
PE_RVA2Ptr(
    _In_ PPE_STRUCT PEStruct,
    _In_ DWORD RVA);

/// <summary>
/// Convert pointer to RVA
/// </summary>
MLE_API
_Success_(return != FALSE)
LOGICAL
NTAPI
PE_Ptr2RVA(
    _In_ PPE_STRUCT PEStruct,
    _In_ PVOID Ptr,
    _Out_ PDWORD RVA);

/// <summary>
/// Convert pointer to offset
/// </summary>
MLE_API
_Success_(return != FALSE)
LOGICAL
NTAPI
PE_Ptr2Offset(
    _In_ PPE_STRUCT PEStruct,
    _In_ PVOID Ptr,
    _Out_ PDWORD Offset);

/// <summary>
/// Get exported name of specified function
/// </summary>
/// <param name="PEStruct">Pointer to the PE_STRUCT structure</param>
/// <param name="Function">Address of function</param>
/// <param name="Name">Pointer to a PCSTR variable to receive the exported name of the function</param>
MLE_API
_Success_(return != FALSE)
LOGICAL
NTAPI
PE_GetExportedName(
    _In_ PPE_STRUCT PEStruct,
    _In_ PVOID Function,
    _Out_ PZPCSTR Name);

/// <summary>
/// Get PE certificate if exists
/// </summary>
MLE_API
_Success_(return != NULL)
_Ret_maybenull_
LPWIN_CERTIFICATE
PE_GetCertificate(
    _In_ PPE_STRUCT PEStruct);

EXTERN_C_END
