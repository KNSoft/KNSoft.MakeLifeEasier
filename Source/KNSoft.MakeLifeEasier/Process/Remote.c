﻿#include "../MakeLifeEasier.inl"

#pragma region Memory R/W

typedef
NTSTATUS
NTAPI
FN_NtWow64ReadVirtualMemory64(
    _In_ HANDLE ProcessHandle,
    _In_ ULONGLONG BaseAddress,
    _Out_writes_bytes_(NumberOfBytesToRead) PVOID Buffer,
    _In_ ULONGLONG NumberOfBytesToRead,
    _Out_opt_ PULONGLONG NumberOfBytesRead);

static FN_NtWow64ReadVirtualMemory64* g_pfnNtWow64ReadVirtualMemory64 = NULL;
static CONST ANSI_STRING g_asNtWow64ReadVirtualMemory64 = RTL_CONSTANT_STRING("NtWow64ReadVirtualMemory64");

NTSTATUS
NTAPI
PS_32Read64Memory(
    _In_ HANDLE ProcessHandle,
    _In_ VOID* POINTER_64 BaseAddress,
    _Out_writes_bytes_(NumberOfBytesToRead) PVOID Buffer,
    _In_ ULONGLONG NumberOfBytesToRead,
    _Out_opt_ PULONGLONG NumberOfBytesRead)
{
    NTSTATUS Status;

    if (g_pfnNtWow64ReadVirtualMemory64 == NULL)
    {
        Status = Sys_LoadProcByName(SysLibNtDll,
                                    &g_asNtWow64ReadVirtualMemory64,
                                    (PVOID*)&g_pfnNtWow64ReadVirtualMemory64);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }
    return g_pfnNtWow64ReadVirtualMemory64(ProcessHandle,
                                           (ULONGLONG)BaseAddress,
                                           Buffer,
                                           NumberOfBytesToRead,
                                           NumberOfBytesRead);
}

#pragma endregion

static
NTSTATUS
PS_GetMachineTypeFromFile(
    _In_ HANDLE ProcessHandle,
    _Out_ PUSHORT MachineType)
{
    NTSTATUS Status;
    ULONG Length;
    BYTE Buffer[sizeof(UNICODE_STRING) + MAX_PATH * sizeof(WCHAR)];
    PUNICODE_STRING ImageFileName;
    HANDLE FileHandle;
    LARGE_INTEGER Offset;
    LONG NtHeaderOffset;

    /* Get process image file path */
    Status = NtQueryInformationProcess(ProcessHandle, ProcessImageFileName, Buffer, sizeof(Buffer), &Length);
    if (Status == STATUS_INFO_LENGTH_MISMATCH)
    {
        ImageFileName = Mem_Alloc(Length);
        if (ImageFileName == NULL)
        {
            return STATUS_NO_MEMORY;
        }
        Status = NtQueryInformationProcess(ProcessHandle, ProcessImageFileName, ImageFileName, Length, NULL);
        if (!NT_SUCCESS(Status))
        {
            Mem_Free(ImageFileName);
            return Status;
        }
    } else if (NT_SUCCESS(Status))
    {
        ImageFileName = (PUNICODE_STRING)Buffer;
    } else
    {
        return Status;
    }

    /* Read file data */
    Status = IO_OpenFile(&FileHandle, ImageFileName, NULL, FILE_READ_DATA | SYNCHRONIZE, FILE_SHARE_READ | FILE_SHARE_WRITE);
    if (!NT_SUCCESS(Status))
    {
        goto _Exit_0;
    }
    Offset.QuadPart = UFIELD_OFFSET(IMAGE_DOS_HEADER, e_lfanew);
    Status = IO_ReadFile(FileHandle, &Offset, &NtHeaderOffset, FIELD_SIZE(IMAGE_DOS_HEADER, e_lfanew));
    if (!NT_SUCCESS(Status))
    {
        goto _Exit_1;
    }
    C_ASSERT(sizeof(*MachineType) == FIELD_SIZE(IMAGE_NT_HEADERS, FileHeader.Machine));
    Offset.QuadPart = NtHeaderOffset + (LONGLONG)FIELD_OFFSET(IMAGE_NT_HEADERS, FileHeader.Machine);
    Status = IO_ReadFile(FileHandle, &Offset, MachineType, sizeof(*MachineType));

_Exit_1:
    NtClose(FileHandle);
_Exit_0:
    if (ImageFileName != (PUNICODE_STRING)Buffer)
    {
        Mem_Free(ImageFileName);
    }
    return Status;
}

NTSTATUS
NTAPI
PS_GetMachineType(
    _In_ HANDLE ProcessHandle,
    _Out_ PUSHORT MachineType)
{
    NTSTATUS Status;
    SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION Arch[8];
    ULONG i, Length;
    SECTION_IMAGE_INFORMATION SecInfo;

    Status = NtQuerySystemInformationEx(SystemSupportedProcessorArchitectures,
                                        &ProcessHandle,
                                        sizeof(ProcessHandle),
                                        Arch,
                                        sizeof(Arch),
                                        &Length);
    if (!NT_SUCCESS(Status))
    {
        goto _Fallback_0;
    }
    for (i = 0; i < Length / sizeof(Arch[0]); i++)
    {
        if (Arch[i].Machine != IMAGE_FILE_MACHINE_UNKNOWN && Arch[i].Process)
        {
            *MachineType = (USHORT)Arch[i].Machine;
            return STATUS_SUCCESS;
        }
    }

_Fallback_0:
    Status = NtQueryInformationProcess(ProcessHandle, ProcessImageInformation, &SecInfo, sizeof(SecInfo), NULL);
    if (!NT_SUCCESS(Status))
    {
        goto _Fallback_1;
    }
    *MachineType = SecInfo.Machine;
    return STATUS_SUCCESS;

_Fallback_1:
    return PS_GetMachineTypeFromFile(ProcessHandle, MachineType);
}

HRESULT
NTAPI
PS_GetRemoteAddressName(
    _In_ HANDLE ProcessHandle,
    _In_ ULONGLONG Address,
    _Outptr_opt_ PUNICODE_STRING* ModulePath,
    _Outptr_opt_result_maybenull_ PUNICODE_STRING* SymbolName,
    _Out_opt_ _When_(SymbolName == NULL, _Null_) PULONGLONG SymbolDisplacement)
{
    HRESULT hr;
    NTSTATUS Status;
    USHORT Bits;
    LDR_DATA_TABLE_ENTRY64 DllEntry64;
    LDR_DATA_TABLE_ENTRY32 DllEntry32;
    PUNICODE_STRING DllPath;
    W32ERROR SymRet;
    DWORD OldSymOptions;
    DWORD64 SymModuleBase;
    BYTE SymInfoBuffer[sizeof(SYMBOL_INFOW) + (MAX_CIDENTIFIERNAME_CCH - 1) * sizeof(WCHAR)];
    PSYMBOL_INFOW SymInfo;
    PUNICODE_STRING SymName;

    /* Get process machine bits */
    Status = PS_GetMachineBits(ProcessHandle, &Bits);
    if (!NT_SUCCESS(Status))
    {
        return HRESULT_FROM_NT(Status);
    }

    /* Get module full path */
    if (Bits != 32)
    {
        Status = PS_GetRemoteModuleEntryByAddress64(ProcessHandle, (VOID* POINTER_64)Address, &DllEntry64);
    } else
    {
        Status = PS_GetRemoteModuleEntryByAddress32(ProcessHandle, (VOID* POINTER_32)Address, &DllEntry32);
    }
    if (!NT_SUCCESS(Status))
    {
        return HRESULT_FROM_NT(Status);
    }
    if (Bits != 32)
    {
        Status = PS_DuplicateUnicodeString64(ProcessHandle, &DllEntry64.FullDllName, &DllPath);
    } else
    {
        Status = PS_DuplicateUnicodeString32(ProcessHandle, &DllEntry32.FullDllName, &DllPath);
    }
    if (!NT_SUCCESS(Status))
    {
        return HRESULT_FROM_NT(Status);
    }
    if (SymbolName == NULL)
    {
        hr = S_OK;
        goto _Exit_0;
    }

    /* Get symbol name */
    if (SymbolDisplacement != NULL)
    {
        *SymbolDisplacement = 0;
    }
    SymName = NULL;
    SymRet = PE_SymInitialize(NULL, FALSE);
    if (SymRet != ERROR_SUCCESS)
    {
        goto _Exit_1;
    }
    SymRet = PE_SymSetOptions(SYMOPT_FAIL_CRITICAL_ERRORS |
                              SYMOPT_NO_PROMPTS |
                              SYMOPT_UNDNAME |
                              SYMOPT_NO_UNQUALIFIED_LOADS |
                              SYMOPT_OMAP_FIND_NEAREST, &OldSymOptions);
    if (SymRet != ERROR_SUCCESS)
    {
        OldSymOptions = MAXDWORD;
    }
    SymRet = PE_SymLoadModule(NULL,
                              DllPath->Buffer,
                              NULL,
                              Bits != 32 ? (DWORD64)DllEntry64.DllBase: (DWORD64)DllEntry32.DllBase,
                              Bits != 32 ? DllEntry64.SizeOfImage : DllEntry32.SizeOfImage,
                              NULL,
                              0,
                              &SymModuleBase);
    if (SymRet != ERROR_SUCCESS)
    {
        goto _Exit_2;
    }
    SymInfo = (PSYMBOL_INFOW)SymInfoBuffer;
    SymInfo->SizeOfStruct = sizeof(*SymInfo);
    C_ASSERT(MAX_CIDENTIFIERNAME_CCH < MAXUSHORT);
    SymInfo->MaxNameLen = MAX_CIDENTIFIERNAME_CCH;
    SymRet = PE_SymFromAddr((DWORD64)Address, SymbolDisplacement, (PSYMBOL_INFOW)SymInfoBuffer);
    if (SymRet != ERROR_SUCCESS)
    {
        goto _Exit_3;
    }

    SymName = NT_AllocStringW((USHORT)SymInfo->NameLen);
    if (SymName == NULL)
    {
        goto _Exit_3;
    }
    memcpy(SymName->Buffer, SymInfo->Name, SymName->Length);
    SymName->Buffer[SymInfo->NameLen] = UNICODE_NULL;

_Exit_3:
    if (SymModuleBase != 0)
    {
        PE_SymUnloadModule(SymModuleBase);
    }
_Exit_2:
    if (OldSymOptions != MAXDWORD)
    {
        PE_SymSetOptions(OldSymOptions, NULL);
    }
    PE_SymCleanup();
_Exit_1:
    *SymbolName = SymName;
    hr = SymName == NULL ? S_FALSE : S_OK;
_Exit_0:
    if (ModulePath != NULL)
    {
        *ModulePath = DllPath;
    } else
    {
        PS_FreeUnicodeString(DllPath);
    }
    return hr;
}
