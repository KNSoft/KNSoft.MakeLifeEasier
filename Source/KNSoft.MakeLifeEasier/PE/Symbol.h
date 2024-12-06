#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

MLE_API
W32ERROR
NTAPI
PE_SymInitialize(
    _In_opt_ PCWSTR UserSearchPath,
    _In_ BOOL fInvadeProcess);

MLE_API
W32ERROR
NTAPI
PE_SymCleanup(VOID);

MLE_API
W32ERROR
NTAPI
PE_SymSetOptions(
    _In_ DWORD SymOptions,
    _Out_opt_ PDWORD OldSymOptions);

MLE_API
W32ERROR
NTAPI
PE_SymLoadModule(
    _In_opt_ HANDLE hFile,
    _In_opt_ PCWSTR ImageName,
    _In_opt_ PCWSTR ModuleName,
    _In_ DWORD64 BaseOfDll,
    _In_ DWORD DllSize,
    _In_opt_ PMODLOAD_DATA Data,
    _In_opt_ DWORD Flags,
    _Out_opt_ PDWORD64 BaseAddress);

MLE_API
W32ERROR
NTAPI
PE_SymUnloadModule(
    _In_ DWORD64 BaseOfDll);

MLE_API
W32ERROR
NTAPI
PE_SymFromAddr(
    _In_ DWORD64 Address,
    _Out_opt_ PDWORD64 Displacement,
    _Inout_ PSYMBOL_INFOW Symbol);

EXTERN_C_END
