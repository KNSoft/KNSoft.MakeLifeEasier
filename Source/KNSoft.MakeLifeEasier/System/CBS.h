#pragma once

#include "../MakeLifeEasier.h"

#include <KNSoft/NDK/Win32/API/CBS/CbsApi.h>

EXTERN_C_START

typedef struct _SYS_CBS_FEATURE_STATE
{
    CBS_APPLICABILITY Applicability;
    CBS_SELECTABILITY Selectability;
    CBS_INSTALL_STATE Current;
    CBS_INSTALL_STATE Intended;
    CBS_INSTALL_STATE Requested;
} SYS_CBS_FEATURE_STATE, *PSYS_CBS_FEATURE_STATE;
typedef const SYS_CBS_FEATURE_STATE* PCSYS_CBS_FEATURE_STATE;

typedef struct _SYS_CBS_FEATURE
{
    ICbsUpdate* Update;
    PCWSTR Name;
    PCWSTR DisplayName;
    PCWSTR Description;
    SYS_CBS_FEATURE_STATE State;
} SYS_CBS_FEATURE, *PSYS_CBS_FEATURE;
typedef const SYS_CBS_FEATURE* PCSYS_CBS_FEATURE;

// Return S_OK to continue, S_FALSE to stop, or an error to abort.
// The update and strings are borrowed and valid only during the callback.
typedef
_Function_class_(SYS_CBS_FEATURE_CALLBACK)
__callback
HRESULT
CALLBACK
SYS_CBS_FEATURE_CALLBACK(
    _In_ PCSYS_CBS_FEATURE Feature,
    _In_opt_ PVOID Context);
typedef SYS_CBS_FEATURE_CALLBACK *PSYS_CBS_FEATURE_CALLBACK;

// ParentSet is an opaque CBS relationship identifier and may be NULL.
typedef
_Function_class_(SYS_CBS_FEATURE_PARENT_CALLBACK)
__callback
HRESULT
CALLBACK
SYS_CBS_FEATURE_PARENT_CALLBACK(
    _In_ PCSYS_CBS_FEATURE Feature,
    _In_ PCWSTR ParentName,
    _In_opt_ PCWSTR ParentSet,
    _In_opt_ PVOID Context);
typedef SYS_CBS_FEATURE_PARENT_CALLBACK *PSYS_CBS_FEATURE_PARENT_CALLBACK;

// The caller must initialize COM before using the low-level session helpers.
MLE_API
HRESULT
NTAPI
Sys_CbsOpenSession(
    _In_ PCWSTR ClientId,
    _Outptr_ ICbsSession** Session);

MLE_API
HRESULT
NTAPI
Sys_CbsCancelSession(
    _In_ ICbsSession* Session,
    _Out_ CBS_REQUIRED_ACTION* RequiredAction);

MLE_API
HRESULT
NTAPI
Sys_CbsOpenPackage(
    _In_ ICbsSession* Session,
    _In_ PCWSTR PackageIdentity,
    _Outptr_ ICbsPackage** Package);

MLE_API
HRESULT
NTAPI
Sys_CbsGetFeatureState(
    _In_ ICbsUpdate* Update,
    _Out_ PSYS_CBS_FEATURE_STATE State);

MLE_API
HRESULT
NTAPI
Sys_CbsEnumeratePackageFeatures(
    _In_ ICbsPackage* Package,
    _In_ CBS_APPLICABILITY Applicability,
    _In_ CBS_SELECTABILITY Selectability,
    _In_ __callback PSYS_CBS_FEATURE_CALLBACK FeatureCallback,
    _In_opt_ __callback PSYS_CBS_FEATURE_PARENT_CALLBACK ParentCallback,
    _In_opt_ PVOID Context);

MLE_API
HRESULT
NTAPI
Sys_CbsEnumerateFeatures(
    _In_ PCWSTR ClientId,
    _In_ __callback PSYS_CBS_FEATURE_CALLBACK FeatureCallback,
    _In_opt_ __callback PSYS_CBS_FEATURE_PARENT_CALLBACK ParentCallback,
    _In_opt_ PVOID Context);

// On an operational failure, the session is cancelled and finalized.
MLE_API
HRESULT
NTAPI
Sys_CbsSetPackageFeaturesEnabled(
    _In_ ICbsSession* Session,
    _In_ ICbsPackage* Package,
    _In_reads_(FeatureCount) PCWSTR const* FeatureNames,
    _In_range_(>, 0) ULONG FeatureCount,
    _In_ LOGICAL Enable,
    _In_ LOGICAL EnableDependencies);

MLE_API
HRESULT
NTAPI
Sys_CbsSetFeaturesEnabled(
    _In_ PCWSTR ClientId,
    _In_reads_(FeatureCount) PCWSTR const* FeatureNames,
    _In_range_(>, 0) ULONG FeatureCount,
    _In_ LOGICAL Enable,
    _In_ LOGICAL EnableDependencies,
    _Out_ CBS_REQUIRED_ACTION* RequiredAction);

MLE_API
HRESULT
NTAPI
Sys_CbsSetFeatureEnabled(
    _In_ PCWSTR ClientId,
    _In_ PCWSTR FeatureName,
    _In_ LOGICAL Enable,
    _In_ LOGICAL EnableDependencies,
    _Out_ CBS_REQUIRED_ACTION* RequiredAction);

EXTERN_C_END
