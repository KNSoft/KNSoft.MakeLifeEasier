#include "../MakeLifeEasier.inl"

C_ASSERT(_BITS == 32 || _BITS == 64);

#if !defined(_WIN64) && (_BITS == 64)
#define PS_ReadMemory PS_32Read64Memory
#else
#define PS_ReadMemory NtReadVirtualMemory
#endif

#if _BITS == 64

#define PS_EnumerateModulesT PS_EnumerateModules64
#define PLDR_ENUM_CALLBACK_T PLDR_ENUM_CALLBACK64
#define PEB_T PEB64
#define POINTER_T POINTER_64
#define PEB_LDR_DATA_T PEB_LDR_DATA64
#define LDR_DATA_TABLE_ENTRY_T LDR_DATA_TABLE_ENTRY64
#define LDR_ENUM_CALLBACK_T LDR_ENUM_CALLBACK64
#define PS_FindModulesByNameT PS_FindModulesByName64
#define ULONG_T ULONG64
#define PS_GetRemoteModuleEntryByAddressT PS_GetRemoteModuleEntryByAddress64
#define PS_DuplicateUnicodeStringT PS_DuplicateUnicodeString64
#define PUNICODE_STRING_T PUNICODE_STRING64
#define PS_GetRemoteAddressNameT PS_GetRemoteAddressName64

#elif _BITS == 32

#define PS_EnumerateModulesT PS_EnumerateModules32
#define PLDR_ENUM_CALLBACK_T PLDR_ENUM_CALLBACK32
#define PEB_T PEB32
#define POINTER_T POINTER_32
#define PEB_LDR_DATA_T PEB_LDR_DATA32
#define LDR_DATA_TABLE_ENTRY_T LDR_DATA_TABLE_ENTRY32
#define LDR_ENUM_CALLBACK_T LDR_ENUM_CALLBACK32
#define PS_FindModulesByNameT PS_FindModulesByName32
#define ULONG_T ULONG32
#define PS_GetRemoteModuleEntryByAddressT PS_GetRemoteModuleEntryByAddress32
#define PS_DuplicateUnicodeStringT PS_DuplicateUnicodeString32
#define PUNICODE_STRING_T PUNICODE_STRING32
#define PS_GetRemoteAddressNameT PS_GetRemoteAddressName32

#endif

/* See also LdrEnumerateLoadedModules, EnumProcessModules */

NTSTATUS
NTAPI
PS_EnumerateModulesT(
    _In_ HANDLE ProcessHandle,
    _In_ PLDR_ENUM_CALLBACK_T EnumProc,
    _In_ PVOID Context)
{
    NTSTATUS Status;
    PROCESS_BASIC_INFORMATION pbi;
    PVOID Wow64Peb;
    PEB_T* POINTER_T PebPtr;
    PEB_LDR_DATA_T* POINTER_T LdrPtr;
    LDR_DATA_TABLE_ENTRY_T* POINTER_T HeadPtr;
    LDR_DATA_TABLE_ENTRY_T* POINTER_T NodePtr;
    LDR_DATA_TABLE_ENTRY_T LdrEntry;
    ULONG Count;
    BOOLEAN Stop;

    /* Prefer Wow64 PEB */
    Status = PS_GetWow64Peb(ProcessHandle, &Wow64Peb);
    if (NT_SUCCESS(Status) && Wow64Peb != NULL)
    {
        PebPtr = (PEB_T * POINTER_T)Wow64Peb;
    } else
    {
        Status = NtQueryInformationProcess(ProcessHandle, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
        PebPtr = (PEB_T * POINTER_T)pbi.PebBaseAddress;
    }

    Status = PS_RWStatus(PS_ReadMemory(ProcessHandle, &PebPtr->Ldr, &LdrPtr, sizeof(LdrPtr), NULL));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    Status = PS_RWStatus(PS_ReadMemory(ProcessHandle,
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
        Status = PS_RWStatus(PS_ReadMemory(ProcessHandle, NodePtr, &LdrEntry, sizeof(LdrEntry), NULL));
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
        NodePtr = (LDR_DATA_TABLE_ENTRY_T * POINTER_T)CONTAINING_RECORD((ULONG_PTR)LdrEntry.InLoadOrderLinks.Flink,
                                                                       LDR_DATA_TABLE_ENTRY_T,
                                                                       InLoadOrderLinks);
    } while (NodePtr != HeadPtr);

    return STATUS_SUCCESS;
}

typedef union _PS_FIND_MODULE
{
#if _BITS == 64
    struct
    {
        union
        {
            VOID* POINTER_64 Address;
        };
        PLDR_DATA_TABLE_ENTRY64 ModuleEntry;
    };
#elif _BITS == 32
    struct
    {
        union
        {
            VOID* POINTER_32 Address;
        };
        PLDR_DATA_TABLE_ENTRY32 ModuleEntry;
    };
#endif
} PS_FIND_MODULE, *PPS_FIND_MODULE;

static
_Function_class_(LDR_ENUM_CALLBACK_T)
VOID
NTAPI
PS_FindModulesByNameT(
    _In_ LDR_DATA_TABLE_ENTRY_T* ModuleInformation,
    _In_ PVOID Parameter,
    _Out_ BOOLEAN* Stop)
{
    PPS_FIND_MODULE Info = (PPS_FIND_MODULE)Parameter;

    if ((ULONG_T)Info->Address >= (ULONG_T)ModuleInformation->DllBase &&
        (ULONG_T)Info->Address < (ULONG_T)ModuleInformation->DllBase + ModuleInformation->SizeOfImage)
    {
        C_ASSERT(sizeof(*Info->ModuleEntry) == sizeof(*ModuleInformation));
        memcpy(Info->ModuleEntry, ModuleInformation, sizeof(*Info->ModuleEntry));
        *Stop = TRUE;
    } else
    {
        *Stop = FALSE;
    }
}

NTSTATUS
NTAPI
PS_GetRemoteModuleEntryByAddressT(
    _In_ HANDLE ProcessHandle,
    _In_ VOID* POINTER_T Address,
    _Out_ LDR_DATA_TABLE_ENTRY_T* ModuleEntry)
{
    PS_FIND_MODULE Info;

    Info.Address = Address;
    Info.ModuleEntry = ModuleEntry;
    return PS_EnumerateModulesT(ProcessHandle, PS_FindModulesByNameT, (PVOID)&Info);
}

NTSTATUS
NTAPI
PS_DuplicateUnicodeStringT(
    _In_ HANDLE ProcessHandle,
    _In_ PUNICODE_STRING_T Src,
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
    Status = PS_RWStatus(PS_ReadMemory(ProcessHandle, (VOID * POINTER_T)Src->Buffer, Buffer, Src->Length, NULL));
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

HRESULT
NTAPI
PS_GetRemoteAddressNameT(
    _In_ HANDLE ProcessHandle,
    _In_ VOID* POINTER_T Address,
    _Outptr_opt_ PUNICODE_STRING* ModulePath,
    _Outptr_opt_ PUNICODE_STRING* SymbolName,
    _Out_opt_ _When_(SymbolName == NULL, _Null_) PULONGLONG SymbolDisplacement)
{
    HRESULT hr;
    NTSTATUS Status;
    LDR_DATA_TABLE_ENTRY_T DllEntry;
    PUNICODE_STRING DllPath;
    W32ERROR SymRet;
    DWORD OldSymOptions;
    DWORD64 SymModuleBase;
    BYTE SymInfoBuffer[sizeof(SYMBOL_INFOW) + (MAX_CIDENTIFIERNAME_CCH - 1) * sizeof(WCHAR)];
    PSYMBOL_INFOW SymInfo = (PSYMBOL_INFOW)SymInfoBuffer;
    ULONG SymNameSize;
    PUNICODE_STRING SymName;
    PWCHAR SymNameString;

    /* Get module full path */
    Status = PS_GetRemoteModuleEntryByAddressT(ProcessHandle, Address, &DllEntry);
    if (!NT_SUCCESS(Status))
    {
        return HRESULT_FROM_NT(Status);
    }
    Status = PS_DuplicateUnicodeStringT(ProcessHandle, &DllEntry.FullDllName, &DllPath);
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
                              (DWORD64)DllEntry.DllBase,
                              DllEntry.SizeOfImage,
                              NULL,
                              0,
                              &SymModuleBase);
    if (SymRet != ERROR_SUCCESS)
    {
        goto _Exit_2;
    }
    SymInfo->SizeOfStruct = sizeof(*SymInfo);
    SymInfo->MaxNameLen = MAX_CIDENTIFIERNAME_CCH;
    SymRet = PE_SymFromAddr((DWORD64)(ULONG_T)Address, SymbolDisplacement, (PSYMBOL_INFOW)SymInfoBuffer);
    if (SymRet != ERROR_SUCCESS)
    {
        goto _Exit_3;
    }
    SymNameSize = SymInfo->NameLen * sizeof(WCHAR);
    SymName = Mem_Alloc(sizeof(UNICODE_STRING) + SymNameSize + sizeof(UNICODE_NULL));
    if (SymName == NULL)
    {
        goto _Exit_3;
    }
    SymNameString = Add2Ptr(SymName, sizeof(UNICODE_STRING));
    memcpy(SymNameString, SymInfo->Name, SymNameSize);
    SymNameString[SymInfo->NameLen] = UNICODE_NULL;
    SymName->Length = (USHORT)SymNameSize;
    SymName->MaximumLength = (USHORT)SymNameSize + sizeof(UNICODE_NULL);
    SymName->Buffer = SymNameString;

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
        PS_FreeDuplicatedUnicodeString(DllPath);
    }
    return hr;
}
