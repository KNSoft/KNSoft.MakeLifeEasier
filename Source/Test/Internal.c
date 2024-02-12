#include "Test.h"

#include <stdio.h>
#include <dpfilter.h>

#pragma region CRT Error Mode

#if !defined(_VC_NODEFAULTLIB)

#include <stdlib.h>
#include <crtdbg.h>

static _invalid_parameter_handler g_pfnOldHandler = NULL;
static int g_iReportMode;

void __cdecl _invalid_parameter_nothrow(
   const wchar_t* expression,
   const wchar_t* function,
   const wchar_t* file,
   unsigned int line,
   uintptr_t pReserved)
{
    DbgPrint("Invalid parameter passed to C runtime function.\n");
}

#endif

VOID SetNtdllCRTErrorMode()
{
#if !defined(_VC_NODEFAULTLIB)
    g_pfnOldHandler = _set_invalid_parameter_handler(_invalid_parameter_nothrow);
    g_iReportMode = _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
#endif
}

VOID RevertCRTErrorMode()
{
#if !defined(_VC_NODEFAULTLIB)
    if (g_iReportMode != -1)
    {
        _CrtSetReportMode(_CRT_ASSERT, g_iReportMode);
    }
    _set_invalid_parameter_handler(g_pfnOldHandler);
#endif
}

#pragma endregion

#pragma region String PrintF

_Success_(
    return > 0 && return < BufferCount
)
ULONG Str_CchVPrintfA(
    _Out_writes_opt_(BufferCount) _Always_(_Post_z_) PSTR CONST Buffer,
    _In_ SIZE_T CONST BufferCount,
    _In_z_ _Printf_format_string_ PCSTR Format,
    va_list ArgList)
{
    int i;

#pragma warning(disable: 4996)
    i = _vsnprintf(Buffer, BufferCount, Format, ArgList);
#pragma warning(default: 4996)
    if (i > 0)
    {
        if (Buffer != NULL && i == BufferCount)
        {
            Buffer[i - 1] = ANSI_NULL;
        }
        return i;
    } else if (i == 0)
    {
        return 0;
    }

#pragma warning(disable: 4996)
    i = _vsnprintf(NULL, 0, Format, ArgList);
#pragma warning(default: 4996)
    if (i > 0)
    {
        if (Buffer != NULL && (SIZE_T)i > BufferCount && BufferCount > 0)
        {
            Buffer[BufferCount - 1] = ANSI_NULL;
        }
        return i;
    }

    return 0;
}

_Success_(
    return > 0 && return < BufferCount
)
ULONG Str_CchVPrintfW(
    _Out_writes_opt_(BufferCount) _Always_(_Post_z_) PWSTR CONST Buffer,
    _In_ SIZE_T CONST BufferCount,
    _In_z_ _Printf_format_string_ PCWSTR Format,
    va_list ArgList)
{
    int i;

#pragma warning(disable: 4996)
    i = _vsnwprintf(Buffer, BufferCount, Format, ArgList);
#pragma warning(default: 4996)
    if (i > 0)
    {
        if (Buffer != NULL && i == BufferCount)
        {
            Buffer[i - 1] = UNICODE_NULL;
        }
        return i;
    } else if (i == 0)
    {
        return 0;
    }

#pragma warning(disable: 4996)
    i = _vsnwprintf(NULL, 0, Format, ArgList);
#pragma warning(default: 4996)
    if (i > 0)
    {
        if (Buffer != NULL && (SIZE_T)i > BufferCount && BufferCount > 0)
        {
            Buffer[BufferCount - 1] = UNICODE_NULL;
        }
        return i;
    }

    return 0;
}

_Success_(
    return > 0 && return < BufferCount
)
ULONG Str_CchPrintfA(
    _Out_writes_opt_(BufferCount) _Always_(_Post_z_) PSTR CONST Buffer,
    _In_ SIZE_T CONST BufferCount,
    _In_z_ _Printf_format_string_ PCSTR Format, ...)
{
    va_list argList;

    va_start(argList, Format);
    return Str_CchVPrintfA(Buffer, BufferCount, Format, argList);
}

_Success_(
    return > 0 && return < BufferCount
)
ULONG Str_CchPrintfW(
    _Out_writes_opt_(BufferCount) _Always_(_Post_z_) PWSTR CONST Buffer,
    _In_ SIZE_T CONST BufferCount,
    _In_z_ _Printf_format_string_ PCWSTR Format, ...)
{
    va_list argList;

    va_start(argList, Format);
    return Str_CchVPrintfW(Buffer, BufferCount, Format, argList);
}

#pragma endregion

#pragma region Print

VOID PrintF(_In_z_ _Printf_format_string_ PCSTR Format, ...)
{
    CHAR szTemp[512];
    va_list argList;
    ULONG uLen;
    HANDLE hStdOut;
    IO_STATUS_BLOCK IoStatusBlock;

    va_start(argList, Format);
    vDbgPrintEx(MAXULONG, DPFLTR_ERROR_LEVEL, Format, argList);
    
    hStdOut = NtCurrentPeb()->ProcessParameters->StandardOutput;
    if (hStdOut != NULL)
    {
        uLen = Str_CchVPrintfA(szTemp, ARRAYSIZE(szTemp), Format, argList);
        if (uLen > 0 && uLen < ARRAYSIZE(szTemp))
        {
            NtWriteFile(hStdOut, NULL, NULL, NULL, &IoStatusBlock, szTemp, uLen, NULL, NULL);
        }
    }
}

static PCSTR g_szDividingLine = "\n**********************************************************************\n";

VOID PrintTitle(_In_z_ PCSTR Name, _In_z_ PCSTR Description)
{
    PrintF(g_szDividingLine);
    PrintF("%hs\n\t%hs", Name, Description);
    PrintF(g_szDividingLine);
}

#pragma endregion

#pragma region NT function wrapper

_Success_(return != FALSE)
BOOL NTAPI NT_InitializePathObjectEx(
    _In_z_ PCWSTR Path,
    _In_opt_ HANDLE RootDirectory,
    _In_ ULONG Attributes,
    _In_opt_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Out_ POBJECT_ATTRIBUTES Object,
    _Out_ PUNICODE_STRING ObjectName,
    _Out_opt_ PRTL_RELATIVE_NAME_U RelativeName)
{
    NTSTATUS Status;

    Status = RtlDosPathNameToNtPathName_U_WithStatus(Path, ObjectName, NULL, RelativeName);
    if (!NT_SUCCESS(Status))
    {
        NtSetLastStatus(Status);
        return FALSE;
    }

    InitializeObjectAttributes(Object, ObjectName, Attributes, RootDirectory, SecurityDescriptor);

    return TRUE;
}

_Success_(return != FALSE)
BOOL NTAPI NT_InitializePathObject(
    _In_z_ PCWSTR Path,
    _In_ BOOL InheritHandle,
    _Out_ POBJECT_ATTRIBUTES Object,
    _Out_ PUNICODE_STRING ObjectName,
    _Out_opt_ PRTL_RELATIVE_NAME_U RelativeName)
{
    ULONG Attributes = OBJ_CASE_INSENSITIVE;

    if (InheritHandle)
    {
        Attributes |= OBJ_INHERIT;
    }

    return NT_InitializePathObjectEx(Path, NULL, Attributes, NULL, Object, ObjectName, RelativeName);
}

HANDLE IO_OpenDevice(_In_ PUNICODE_STRING DeviceName, _In_ ACCESS_MASK DesiredAccess)
{
    NTSTATUS Status;
    HANDLE DeviceHandle;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttribute = RTL_CONSTANT_OBJECT_ATTRIBUTES(DeviceName, OBJ_CASE_INSENSITIVE);

    Status = NtOpenFile(&DeviceHandle,
                        DesiredAccess,
                        &ObjectAttribute,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_NON_DIRECTORY_FILE);
    if (!NT_SUCCESS(Status))
    {
        NtSetLastStatus(Status);
        return NULL;
    }

    return DeviceHandle;
}

_Ret_maybenull_
PSYSTEM_FIRMWARE_TABLE_INFORMATION NT_GetFirmwareTable(
    ULONG ProviderSignature,
    ULONG TableID,
    SYSTEM_FIRMWARE_TABLE_ACTION Action)
{
    NTSTATUS Status;
    ULONG Length;
    SYSTEM_FIRMWARE_TABLE_INFORMATION FirmwareInfo;
    PSYSTEM_FIRMWARE_TABLE_INFORMATION Buffer;

    FirmwareInfo.ProviderSignature = ProviderSignature;
    FirmwareInfo.Action = Action;
    FirmwareInfo.TableID = TableID;
    FirmwareInfo.TableBufferLength = 0;

    Status = NtQuerySystemInformation(SystemFirmwareTableInformation, &FirmwareInfo, sizeof(FirmwareInfo), &Length);
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        NtSetLastStatus(Status);
        return NULL;
    }

    _Analysis_assume_(Length >= sizeof(FirmwareInfo));
    Buffer = RtlAllocateHeap(CURRENT_PROCESS_HEAP, 0, Length);
    if (Buffer == NULL)
    {
        return NULL;
    }

    RtlCopyMemory(Buffer, &FirmwareInfo, sizeof(FirmwareInfo));
    Status = NtQuerySystemInformation(SystemFirmwareTableInformation, Buffer, Length, &Length);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(CURRENT_PROCESS_HEAP, 0, Buffer);
        return NULL;
    }

    return Buffer;
}

#pragma endregion
