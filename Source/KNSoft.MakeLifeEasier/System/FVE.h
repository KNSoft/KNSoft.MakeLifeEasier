#pragma once

#include "../MakeLifeEasier.h"

#include <KNSoft/NDK/Win32/API/FveApi.h>

EXTERN_C_START

#define SYS_FVE_DISABLE_COUNT_DEFAULT MAXULONG

// Return S_OK to continue, S_FALSE to stop, or an error to abort.
// VolumeName is borrowed and valid only during the callback.
typedef
_Function_class_(SYS_FVE_VOLUME_CALLBACK)
__callback
HRESULT
CALLBACK
SYS_FVE_VOLUME_CALLBACK(
    _In_ PCWSTR VolumeName,
    _In_ FVE_DEVICE_TYPE DeviceType,
    _In_opt_ PVOID Context);
typedef SYS_FVE_VOLUME_CALLBACK *PSYS_FVE_VOLUME_CALLBACK;

MLE_API
HRESULT
NTAPI
Sys_FveEnumerateVolumes(
    _In_ __callback PSYS_FVE_VOLUME_CALLBACK Callback,
    _In_opt_ PVOID Context);

MLE_API
HRESULT
NTAPI
Sys_FveGetStatus(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PFVE_STATUS_V9 Status);

// Free the returned array with Mem_Free.
MLE_API
HRESULT
NTAPI
Sys_FveGetAuthMethodGuids(
    _In_ HANDLE FveVolumeHandle,
    _Outptr_result_buffer_maybenull_(*AuthMethodCount) PGUID* AuthMethodGuids,
    _Out_ PUINT AuthMethodCount);

MLE_API
HRESULT
NTAPI
Sys_FveGetAuthMethodInformation(
    _In_ HANDLE FveVolumeHandle,
    _In_opt_ PCGUID AuthMethodGuid,
    _In_ ULONG QueryFlags,
    _Outptr_result_bytebuffer_(*BufferSize) PFVE_AUTH_INFORMATION* Information,
    _Out_ PSIZE_T BufferSize);

MLE_API
VOID
NTAPI
Sys_FveFreeAuthMethodInformation(
    _Frees_ptr_opt_ PFVE_AUTH_INFORMATION Information,
    _In_ SIZE_T BufferSize);

MLE_API
HRESULT
NTAPI
Sys_FveAddRecoveryPasswordProtector(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCWSTR RecoveryPassword,
    _In_opt_ PCWSTR Description,
    _Out_ PGUID AuthMethodGuid);

MLE_API
HRESULT
NTAPI
Sys_FveUnlockWithRecoveryPassword(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCWSTR RecoveryPassword);

MLE_API
HRESULT
NTAPI
Sys_FveDisableProtectors(
    _In_ HANDLE FveVolumeHandle,
    // Use SYS_FVE_DISABLE_COUNT_DEFAULT to select the system default.
    _In_ ULONG DisableCount);

MLE_API
HRESULT
NTAPI
Sys_FveEnableProtectors(
    _In_ HANDLE FveVolumeHandle);

MLE_API
HRESULT
NTAPI
Sys_FveDeleteProtector(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCGUID AuthMethodGuid);

MLE_API
HRESULT
NTAPI
Sys_FveEncrypt(
    _In_ HANDLE FveVolumeHandle,
    _In_ FVE_LEGACY_METHOD FveMethod,
    _In_ LOGICAL DataOnly);

MLE_API
HRESULT
NTAPI
Sys_FveDecrypt(
    _In_ HANDLE FveVolumeHandle);

EXTERN_C_END
