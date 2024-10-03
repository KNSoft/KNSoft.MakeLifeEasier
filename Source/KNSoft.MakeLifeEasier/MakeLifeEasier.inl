#pragma once

#define OEMRESOURCE
#define STRICT_TYPED_ITEMIDS

#ifdef MSB_CONFIGURATIONTYPE_EXE
#define _USE_COMMCTL60
#endif

#include <KNSoft/NDK/NDK.h>

#include <stdint.h>

#ifdef _WINDLL
#define MLE_API DECLSPEC_EXPORT
#else
#define MLE_API
#endif

#include "MakeLifeEasier.h"

#pragma region Resource Access Support

/*
 * Support access resource for both of Dll and Lib
 * 
 * Caution: Resource support for Lib is powered by KNSoft.Precomp4C,
 *   which is very incomplete and limited.
 */

#include "resource.h"

#ifdef MSB_CONFIGURATIONTYPE_LIB
#include "Resource.rc.g.h"
#endif

FORCEINLINE
NTSTATUS
Mlep_AccessResource(
    _In_ PWSTR Type,
    _In_ PWSTR Name,
    _In_  ULONG_PTR Language,
    _Out_ PVOID* Resource,
    _Out_ PULONG Length)
{

#ifndef MSB_CONFIGURATIONTYPE_LIB

    const LDR_RESOURCE_INFO ResInfo = {
        .Type = (ULONG_PTR)Type,
        .Name = (ULONG_PTR)Name,
        .Language = Language
    };
    
    return PE_AccessResource(&__ImageBase, (PLDR_RESOURCE_INFO)&ResInfo, Resource, Length);

#else

    return Precomp4C_Res2C_AccessResource(Precomp4C_Res2C_Resource,
                                          ARRAYSIZE(Precomp4C_Res2C_Resource),
                                          (PCWSTR)Type,
                                          (PCWSTR)Name,
                                          (USHORT)Language,
                                          Resource,
                                          (PUINT)Length) ? STATUS_SUCCESS : STATUS_RESOURCE_LANG_NOT_FOUND;

#endif
}

#pragma endregion

#pragma region I18N Support

#include "I18N.xml.g.h"

EXTERN_C
PCWSTR
Mlep_GetString(
    _In_ INT Index);

#pragma endregion
