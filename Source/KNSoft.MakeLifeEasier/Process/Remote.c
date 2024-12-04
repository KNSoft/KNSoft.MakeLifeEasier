#include "../MakeLifeEasier.inl"

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
    Offset.QuadPart = NtHeaderOffset + FIELD_OFFSET(IMAGE_NT_HEADERS, FileHeader.Machine);
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

/* See also LdrEnumerateLoadedModules, EnumProcessModules */

NTSTATUS
NTAPI
PS_EnumerateModules64(
    _In_ HANDLE ProcessHandle,
    _In_ PLDR_ENUM_CALLBACK64 EnumProc,
    _In_ PVOID Context)
{
    NTSTATUS Status;
    PROCESS_BASIC_INFORMATION pbi;
    ULONG_PTR Wow64Peb;
    PEB64* POINTER_64 PebPtr;
    PEB_LDR_DATA64* POINTER_64 LdrPtr;
    LDR_DATA_TABLE_ENTRY64* POINTER_64 HeadPtr;
    LDR_DATA_TABLE_ENTRY64* POINTER_64 NodePtr;
    LDR_DATA_TABLE_ENTRY64 LdrEntry;
    ULONG Count;
    BOOLEAN Stop;

    /* Prefer Wow64 PEB */
    Status = NtQueryInformationProcess(ProcessHandle, ProcessWow64Information, &Wow64Peb, sizeof(Wow64Peb), NULL);
    if (NT_SUCCESS(Status) && Wow64Peb != 0)
    {
        PebPtr = (PEB64 * POINTER_64)Wow64Peb;
    } else
    {
        Status = NtQueryInformationProcess(ProcessHandle, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
        PebPtr = (PEB64 * POINTER_64)pbi.PebBaseAddress;
    }

    Status = PS_RWStatus(PS_ReadMemory64(ProcessHandle, &PebPtr->Ldr, &LdrPtr, sizeof(LdrPtr), NULL));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    Status = PS_RWStatus(PS_ReadMemory64(ProcessHandle,
                                         &LdrPtr->InLoadOrderModuleList.Flink,
                                         &HeadPtr,
                                         sizeof(HeadPtr),
                                         NULL));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    NodePtr = HeadPtr;
    Count = 0;
    do
    {
        Status = PS_RWStatus(PS_ReadMemory64(ProcessHandle, NodePtr, &LdrEntry, sizeof(LdrEntry), NULL));
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
        EnumProc(&LdrEntry, Context, &Stop);
        if (Stop)
        {
            break;
        }
        if (++Count >= 10000)
        {
            return STATUS_INVALID_HANDLE;
        }
        NodePtr = CONTAINING_RECORD(LdrEntry.InLoadOrderLinks.Flink, LDR_DATA_TABLE_ENTRY64, InLoadOrderLinks);
    } while (NodePtr != HeadPtr);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PS_EnumerateModules32(
    _In_ HANDLE ProcessHandle,
    _In_ PLDR_ENUM_CALLBACK32 EnumProc,
    _In_ PVOID Context)
{
    NTSTATUS Status;
    PROCESS_BASIC_INFORMATION pbi;
    ULONG_PTR Wow64Peb;
    PEB32* POINTER_32 PebPtr;
    PEB_LDR_DATA32* POINTER_32 LdrPtr;
    LDR_DATA_TABLE_ENTRY32* POINTER_32 HeadPtr;
    LDR_DATA_TABLE_ENTRY32* POINTER_32 NodePtr;
    LDR_DATA_TABLE_ENTRY32 LdrEntry;
    ULONG Count;
    BOOLEAN Stop;

    /* Prefer Wow64 PEB */
    Status = NtQueryInformationProcess(ProcessHandle, ProcessWow64Information, &Wow64Peb, sizeof(Wow64Peb), NULL);
    if (NT_SUCCESS(Status))
    {
        PebPtr = (PEB32 * POINTER_32)Wow64Peb;
    } else
    {
        Status = NtQueryInformationProcess(ProcessHandle, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
        PebPtr = (PEB32 * POINTER_32)pbi.PebBaseAddress;
    }

    Status = PS_RWStatus(NtReadVirtualMemory(ProcessHandle, &PebPtr->Ldr, &LdrPtr, sizeof(LdrPtr), NULL));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    Status = PS_RWStatus(NtReadVirtualMemory(ProcessHandle,
                                             &LdrPtr->InLoadOrderModuleList.Flink,
                                             &HeadPtr,
                                             sizeof(HeadPtr),
                                             NULL));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    NodePtr = HeadPtr;
    Count = 0;
    do
    {
        Status = PS_RWStatus(NtReadVirtualMemory(ProcessHandle, NodePtr, &LdrEntry, sizeof(LdrEntry), NULL));
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
        EnumProc(&LdrEntry, Context, &Stop);
        if (Stop)
        {
            break;
        }
        if (++Count >= 10000)
        {
            return STATUS_INVALID_HANDLE;
        }
        NodePtr = (LDR_DATA_TABLE_ENTRY32 * POINTER_32)CONTAINING_RECORD((ULONG_PTR)LdrEntry.InLoadOrderLinks.Flink,
                                                                        LDR_DATA_TABLE_ENTRY32,
                                                                        InLoadOrderLinks);
    } while (NodePtr != HeadPtr);

    return STATUS_SUCCESS;
}

typedef struct _PS_FIND_MODULE
{
    union
    {
        VOID* POINTER_64 Address64;
        VOID* POINTER_32 Address32;
    };
    union
    {
        PLDR_DATA_TABLE_ENTRY64 ModuleEntry64;
        PLDR_DATA_TABLE_ENTRY32 ModuleEntry32;
    };
} PS_FIND_MODULE, *PPS_FIND_MODULE;

static
_Function_class_(LDR_ENUM_CALLBACK64)
VOID
NTAPI
PS_FindModulesByName64(
    _In_ PLDR_DATA_TABLE_ENTRY64 ModuleInformation,
    _In_ PVOID Parameter,
    _Out_ BOOLEAN* Stop)
{
    PPS_FIND_MODULE Info = (PPS_FIND_MODULE)Parameter;

    if ((ULONGLONG)Info->Address64 >= (ULONGLONG)ModuleInformation->DllBase &&
        (ULONGLONG)Info->Address64 < (ULONGLONG)ModuleInformation->DllBase + ModuleInformation->SizeOfImage)
    {
        C_ASSERT(sizeof(*Info->ModuleEntry64) == sizeof(*ModuleInformation));
        memcpy(Info->ModuleEntry64, ModuleInformation, sizeof(*Info->ModuleEntry64));
        *Stop = TRUE;
    } else
    {
        *Stop = FALSE;
    }
}

static
_Function_class_(LDR_ENUM_CALLBACK32)
VOID
NTAPI
PS_FindModulesByName32(
    _In_ PLDR_DATA_TABLE_ENTRY32 ModuleInformation,
    _In_ PVOID Parameter,
    _Out_ BOOLEAN* Stop)
{
    PPS_FIND_MODULE Info = (PPS_FIND_MODULE)Parameter;

    if ((ULONGLONG)Info->Address32 >= (ULONGLONG)ModuleInformation->DllBase &&
        (ULONGLONG)Info->Address32 < (ULONGLONG)ModuleInformation->DllBase + ModuleInformation->SizeOfImage)
    {
        C_ASSERT(sizeof(*Info->ModuleEntry32) == sizeof(*ModuleInformation));
        memcpy(Info->ModuleEntry32, ModuleInformation, sizeof(*Info->ModuleEntry32));
        *Stop = TRUE;
    } else
    {
        *Stop = FALSE;
    }
}

NTSTATUS
NTAPI
PS_GetRemoteModuleEntryByAddress64(
    _In_ HANDLE ProcessHandle,
    _In_ VOID* POINTER_64 Address,
    _Out_ PLDR_DATA_TABLE_ENTRY64 ModuleEntry)
{
    PS_FIND_MODULE Info;

    Info.Address64 = Address;
    Info.ModuleEntry64 = ModuleEntry;
    return PS_EnumerateModules64(ProcessHandle, PS_FindModulesByName64, (PVOID)&Info);
}

NTSTATUS
NTAPI
PS_GetRemoteModuleEntryByAddress32(
    _In_ HANDLE ProcessHandle,
    _In_ VOID* POINTER_32 Address,
    _Out_ PLDR_DATA_TABLE_ENTRY32 ModuleEntry)
{
    PS_FIND_MODULE Info;

    Info.Address32 = Address;
    Info.ModuleEntry32 = ModuleEntry;
    return PS_EnumerateModules32(ProcessHandle, PS_FindModulesByName32, (PVOID)&Info);
}

NTSTATUS
NTAPI
PS_DuplicateUnicodeString64(
    _In_ HANDLE ProcessHandle,
    _In_ PUNICODE_STRING64 Src,
    _Out_ PUNICODE_STRING* Dst)
{
    NTSTATUS Status;
    PUNICODE_STRING String;
    PWSTR Buffer;

    String = Mem_Alloc(sizeof(UNICODE_STRING) + Src->Length + sizeof(UNICODE_NULL));
    if (String == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    Buffer = Add2Ptr(String, sizeof(UNICODE_STRING));
    Status = PS_RWStatus(PS_ReadMemory64(ProcessHandle, (VOID * POINTER_64)Src->Buffer, Buffer, Src->Length, NULL));
    if (!NT_SUCCESS(Status))
    {
        Mem_Free(String);
        return Status;
    }

    Buffer[Src->Length / sizeof(WCHAR)] = UNICODE_NULL;
    String->Length = Src->Length;
    String->MaximumLength = Src->Length + sizeof(UNICODE_NULL);
    String->Buffer = Buffer;
    *Dst = String;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PS_DuplicateUnicodeString32(
    _In_ HANDLE ProcessHandle,
    _In_ PUNICODE_STRING32 Src,
    _Out_ PUNICODE_STRING* Dst)
{
    NTSTATUS Status;
    PUNICODE_STRING String;
    PWSTR Buffer;

    String = Mem_Alloc(sizeof(UNICODE_STRING) + Src->Length + sizeof(UNICODE_NULL));
    if (String == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    Buffer = Add2Ptr(String, sizeof(UNICODE_STRING));
    Status = PS_RWStatus(NtReadVirtualMemory(ProcessHandle, (PVOID)(ULONG_PTR)Src->Buffer, Buffer, Src->Length, NULL));
    if (!NT_SUCCESS(Status))
    {
        Mem_Free(String);
        return Status;
    }

    Buffer[Src->Length / sizeof(WCHAR)] = UNICODE_NULL;
    String->Length = Src->Length;
    String->MaximumLength = Src->Length + sizeof(UNICODE_NULL);
    String->Buffer = Buffer;
    *Dst = String;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PS_GetRemoteAddressName64(
    _In_ HANDLE ProcessHandle,
    _In_ VOID* POINTER_64 Address,
    _Outptr_opt_ PUNICODE_STRING* ModuleName,
    _Outptr_opt_ PUNICODE_STRING* SymbolName,
    _Out_opt_ PULONG SymbolOffset)
{
    NTSTATUS Status;
    LDR_DATA_TABLE_ENTRY64 ModuleEntry;

    Status = PS_GetRemoteModuleEntryByAddress64(ProcessHandle, Address, &ModuleEntry);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    if (ModuleName != NULL)
    {
        Status = PS_DuplicateUnicodeString64(ProcessHandle, &ModuleEntry.BaseDllName, ModuleName);
    }

    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
PS_GetRemoteAddressName32(
    _In_ HANDLE ProcessHandle,
    _In_ VOID* POINTER_32 Address,
    _Outptr_opt_ PUNICODE_STRING* ModuleName,
    _Outptr_opt_ PUNICODE_STRING* SymbolName,
    _Out_opt_ PULONG SymbolOffset)
{
    NTSTATUS Status;
    LDR_DATA_TABLE_ENTRY32 ModuleEntry;

    Status = PS_GetRemoteModuleEntryByAddress32(ProcessHandle, Address, &ModuleEntry);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    if (ModuleName != NULL)
    {
        Status = PS_DuplicateUnicodeString32(ProcessHandle, &ModuleEntry.BaseDllName, ModuleName);
    }

    // TODO

    return STATUS_NOT_IMPLEMENTED;
}
