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

    String = NT_AllocStringW(Src->Length / sizeof(WCHAR));
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
