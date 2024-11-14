#pragma once

#define OEMRESOURCE
#define STRICT_TYPED_ITEMIDS

#ifdef MSB_CONFIGURATIONTYPE_EXE
#define _USE_COMMCTL60
#endif

#include <KNSoft/NDK/NDK.h>

#include <stdint.h>

#include "Resource.Embedded.h"
#include "Resource.Embedded.rc.g.h"
#include "I18N.xml.g.h"

#ifdef _WINDLL
#define MLE_API DECLSPEC_EXPORT
#else
#define MLE_API
#endif

#include "MakeLifeEasier.h"

#pragma comment(lib, "KNSoft.NDK.WinAPI.lib")

EXTERN_C_START

PCWSTR
Mlep_GetStringEx(
    _In_ INT Index);

#define Mlep_GetString(x) Mlep_GetStringEx(Precomp4C_I18N_All_##x)

HRESULT
Mlep_DlgBox(
    _In_ PCWSTR DlgResName,
    _In_opt_ HWND Owner,
    _In_opt_ DLGPROC DlgProc,
    _In_opt_ LPARAM InitParam);

EXTERN_C_END
