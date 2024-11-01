#pragma once

#include "../MakeLifeEasier.h"

#include <WinTrust.h>

EXTERN_C_START

#pragma region Resource Access

FORCEINLINE
_Success_(return != NULL)
PCWSTR
PE_FindMessage(
    _In_ PVOID DllHandle,
    _In_ ULONG LanguageId,
    _In_ ULONG MessageId)
{
    PMESSAGE_RESOURCE_ENTRY lpstMRE;

    return NT_SUCCESS(
        RtlFindMessage(DllHandle,
                       (ULONG)(ULONG_PTR)RT_MESSAGETABLE,
                       LanguageId,
                       MessageId,
                       &lpstMRE)) && (lpstMRE->Flags & MESSAGE_RESOURCE_UNICODE) ? (PCWSTR)lpstMRE->Text : NULL;
}

FORCEINLINE
NTSTATUS
PE_AccessResource(
    _In_ PVOID Base,
    _In_ PLDR_RESOURCE_INFO Info,
    _Out_ PVOID* Resource,
    _Out_ PULONG Length)
{
    NTSTATUS Status;
    PIMAGE_RESOURCE_DATA_ENTRY DataEntry;

    Status = LdrFindResource_U(Base, Info, RESOURCE_DATA_LEVEL, &DataEntry);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    return LdrAccessResource(Base, DataEntry, Resource, Length);
}

#pragma endregion

#pragma region PE Resolve

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

MLE_API
_Success_(return != FALSE)
LOGICAL
NTAPI
PE_ResolveOnline(
    _Out_ PPE_STRUCT PEStruct,
    _In_reads_bytes_(ImageSize) PVOID ImageBase,
    _In_ ULONG ImageSize);

MLE_API
_Success_(return != FALSE)
LOGICAL
NTAPI
PE_ResolveOffline(
    _Out_ PPE_STRUCT PEStruct,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize);

#define PE_GetOptionalHeaderValue(pe, m) (\
    (pe)->Bits == 64 ? (pe)->OptionalHeader64->m : (\
        (pe)->Bits == 32 ? (pe)->OptionalHeader32->m : (\
            (pe)->Bits == 0 ? (pe)->OptionalHeaderRom->m :\
                (__fastfail(FAST_FAIL_INVALID_ARG), 0)\
            )\
         )\
    )

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

MLE_API
_Success_(return != NULL)
_Ret_maybenull_
PIMAGE_SECTION_HEADER
NTAPI
PE_GetSectionByRVA(
    _In_ PPE_STRUCT PEStruct,
    _In_ DWORD RVA);

MLE_API
_Success_(return != NULL)
_Ret_maybenull_
PIMAGE_SECTION_HEADER
NTAPI
PE_GetSectionByOffset(
    _In_ PPE_STRUCT PEStruct,
    _In_ DWORD Offset);

MLE_API
_Success_(return != NULL)
_Ret_maybenull_
PVOID
NTAPI
PE_RVA2Ptr(
    _In_ PPE_STRUCT PEStruct,
    _In_ DWORD RVA);

MLE_API
_Success_(return != FALSE)
LOGICAL
NTAPI
PE_Ptr2RVA(
    _In_ PPE_STRUCT PEStruct,
    _In_ PVOID Ptr,
    _Out_ PDWORD RVA);

MLE_API
_Success_(return != FALSE)
LOGICAL
NTAPI
PE_Ptr2Offset(
    _In_ PPE_STRUCT PEStruct,
    _In_ PVOID Ptr,
    _Out_ PDWORD Offset);

MLE_API
_Success_(return != FALSE)
LOGICAL
NTAPI
PE_GetExportedName(
    _In_ PPE_STRUCT PEStruct,
    _In_ PVOID Function,
    _Out_ PZPCSTR Name);

MLE_API
_Success_(return != NULL)
_Ret_maybenull_
LPWIN_CERTIFICATE
PE_GetCertificate(
    _In_ PPE_STRUCT PEStruct);

#pragma endregion

EXTERN_C_END
