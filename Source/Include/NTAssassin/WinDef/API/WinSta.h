#pragma once

#include "../Addendum/winsta.h"

EXTERN_C_START

NTSYSAPI
BOOL
NTAPI
WinStationEnumerateW(
    _In_opt_ HANDLE ServerHandle,
    _Out_ PSESSIONID *SessionIds,
    _Out_ PULONG Count);

NTSYSAPI
BOOL
NTAPI
WinStationFreeMemory(
    _Frees_ptr_ PVOID Buffer);

NTSYSAPI
BOOLEAN
NTAPI
WinStationQueryInformationW(
  _In_opt_ HANDLE ServerHandle,
  _In_ ULONG LogonId,
  _In_ WINSTATIONINFOCLASS WinStationInformationClass,
  _Out_ PVOID WinStationInformation,
  _In_ ULONG WinStationInformationLength,
  _Out_ PULONG ReturnLength);

EXTERN_C_END
