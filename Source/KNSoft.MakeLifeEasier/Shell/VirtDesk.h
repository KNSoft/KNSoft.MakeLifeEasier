#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

MLE_API
HRESULT
NTAPI
Shell_CreateImmersiveISP(
    _Outptr_ IServiceProvider** ImmersiveShellISP);

MLE_API
HRESULT
NTAPI
Shell_CreateIVirtualDesktopManagerInternal(
    _In_ IServiceProvider* ImmersiveShellISP,
    _Outptr_ IVirtualDesktopManagerInternal** Ptr,
    _Outptr_opt_ LPIID* EffetiveIID);

MLE_API
HRESULT
NTAPI
Shell_CreateIApplicationViewCollection(
    _In_ IServiceProvider* ImmersiveShellISP,
    _Outptr_ IApplicationViewCollection** Ptr,
    _Outptr_opt_ LPIID* EffetiveIID);

MLE_API
HRESULT
NTAPI
Shell_CreateIVirtualDesktopPinnedApps(
    _In_ IServiceProvider* ImmersiveShellISP,
    _Outptr_ IVirtualDesktopPinnedApps** Ptr);

/* Return FALSE to stop enumeration */
typedef
_Function_class_(SHELL_ENUM_VIRTUALDESKTOP_PROC)
__callback
LOGICAL
CALLBACK
SHELL_ENUM_VIRTUALDESKTOP_PROC(
    _In_ IVirtualDesktop* VirtualDesktop,
    _In_ UINT Index,
    _In_opt_ PVOID Context);
typedef SHELL_ENUM_VIRTUALDESKTOP_PROC *PSHELL_ENUM_VIRTUALDESKTOP_PROC;

/*
 * Return S_FALSE if enumeration is stopped by Callback.
 * Support Win11 24H2 (26100) and above, due to IVirtualDesktopManagerInternal.
 */
HRESULT
NTAPI
Shell_EnumVirtualDesktops(
    _In_ IVirtualDesktopManagerInternal* VDMI,
    _In_ PSHELL_ENUM_VIRTUALDESKTOP_PROC Callback,
    _In_opt_ PVOID Context);

EXTERN_C_END
