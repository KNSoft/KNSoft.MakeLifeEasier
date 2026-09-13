#pragma once

#include "../../MakeLifeEasier.h"

#include <Unknwn.h>

BOOL
WINAPI
UI_AxHostInitialize(VOID);

PCWSTR
WINAPI
UI_AxHostClassName(VOID);

HRESULT
WINAPI
UI_AxHostGetControl(
    _In_ HWND Window,
    _Outptr_ IUnknown** Control);
