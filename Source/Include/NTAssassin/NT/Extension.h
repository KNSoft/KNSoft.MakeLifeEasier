#pragma once

#include "MinDef.h"

#include "Types.h"

#pragma region WinSDK

#define DECLSPEC_EXPORT __declspec(dllexport)
typedef unsigned __int64 QWORD, near *PQWORD, far *LPQWORD;

// Makes a DWORD value by LOWORD and HIWORD
#define MAKEDWORD(l, h) ((DWORD)(((WORD)(((DWORD_PTR)(l)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(h)) & 0xffff))) << 16))
#define MAKEQWORD(l, h) ((QWORD)(((DWORD)(((DWORD_PTR)(l)) & 0xffffffff)) | ((QWORD)((DWORD)(((DWORD_PTR)(h)) & 0xffffffff))) << 32))

#pragma endregion

#pragma region TEB/PEB fast access

#if defined(_M_X64)

#define ReadTeb(m) (\
    RTL_FIELD_SIZE(TEB, m) == sizeof(ULONGLONG) ? __readgsqword(UFIELD_OFFSET(TEB, m)) : (\
        RTL_FIELD_SIZE(TEB, m) == sizeof(ULONG) ? __readgsdword(UFIELD_OFFSET(TEB, m)) : (\
            RTL_FIELD_SIZE(TEB, m) == sizeof(USHORT) ? __readgsword(UFIELD_OFFSET(TEB, m)) :\
                __readgsbyte(UFIELD_OFFSET(TEB, m))\
        )\
    )\
)

#define WriteTeb(m, val) (\
    RTL_FIELD_SIZE(TEB, m) == sizeof(ULONGLONG) ? __writegsqword(UFIELD_OFFSET(TEB, m), (ULONGLONG)(val)) : (\
        RTL_FIELD_SIZE(TEB, m) == sizeof(ULONG) ? __writegsdword(UFIELD_OFFSET(TEB, m), (ULONG)(val)) : (\
            RTL_FIELD_SIZE(TEB, m) == sizeof(USHORT) ? __writegsword(UFIELD_OFFSET(TEB, m), (USHORT)(val)) :\
                __writegsbyte(UFIELD_OFFSET(TEB, m), (UCHAR)(val))\
        )\
    )\
)

#elif defined(_M_IX86)

#define ReadTeb(m) (\
    RTL_FIELD_SIZE(TEB, m) == sizeof(ULONG) ? __readfsdword(UFIELD_OFFSET(TEB, m)) : (\
        RTL_FIELD_SIZE(TEB, m) == sizeof(USHORT) ? __readfsword(UFIELD_OFFSET(TEB, m)) :\
            __readfsbyte(UFIELD_OFFSET(TEB, m))\
    )\
)

#define WriteTeb(m, val) (\
    RTL_FIELD_SIZE(TEB, m) == sizeof(ULONG) ? __writefsdword(UFIELD_OFFSET(TEB, m), (ULONG)(val)) : (\
        RTL_FIELD_SIZE(TEB, m) == sizeof(USHORT) ? __writefsword(UFIELD_OFFSET(TEB, m), (USHORT)(val)) :\
            __writefsbyte(UFIELD_OFFSET(TEB, m), (UCHAR)(val))\
    )\
)

#else

#define ReadTeb(m) (NtCurrentTeb()->m)
#define WriteTeb(m, val) (NtCurrentTeb()->m = (val))

#endif

#define NtCurrentPeb() ((PPEB)ReadTeb(ProcessEnvironmentBlock))

#pragma endregion

#pragma region Error codes

/* Gets or sets the last error */

#define NtGetLastError() ((ULONG)ReadTeb(LastErrorValue))
#define NtSetLastError(Error) WriteTeb(LastErrorValue, Error)

/* Gets or sets the last status */

#define NtGetLastStatus() ((NTSTATUS)(ReadTeb(LastStatusValue)))
#define NtSetLastStatus(Status) WriteTeb(LastStatusValue, Status)

/*
 * Error code conversion (NOT translation) Win32 Error/NTSTATUS/HRESULT 
 * HRESULT_FROM_WIN32 / NTSTATUS_FROM_WIN32 / HRESULT_FROM_NT
 */

#pragma endregion

#pragma region Current runtime information

#define NtGetCurrentProcessId() ((HANDLE)ReadTeb(ClientId.UniqueProcess))
#define NtGetCurrentThreadId() ((HANDLE)ReadTeb(ClientId.UniqueThread))
#define NtGetCurrentDirectory() ((PCURDIR)&NtCurrentPeb()->ProcessParameters->CurrentDirectory)
#define NtGetCurrentImageBase() (NtCurrentPeb()->ImageBaseAddress)
#define NtGetNtdllBase() (CONTAINING_RECORD(NtCurrentPeb()->Ldr->InInitializationOrderModuleList.Flink, LDR_DATA_TABLE_ENTRY, InInitializationOrderModuleList)->DllBase)
#define NtGetProcessHeap() (NtCurrentPeb()->ProcessHeap)

#pragma endregion

#pragma region Context

#if defined(_M_IX86)
#define CONTEXT_PC Eip
#elif defined(_M_X64)
#define CONTEXT_PC Rip
#elif defined(_M_ARM64)
#define CONTEXT_PC Pc
#endif

#pragma endregion
