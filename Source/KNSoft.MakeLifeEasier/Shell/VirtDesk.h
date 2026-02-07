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
    _Outptr_ IVirtualDesktopManagerInternal** Ptr);

MLE_API
HRESULT
NTAPI
Shell_CreateIApplicationViewCollection(
    _In_ IServiceProvider* ImmersiveShellISP,
    _Outptr_ IApplicationViewCollection** Ptr);

EXTERN_C_END
