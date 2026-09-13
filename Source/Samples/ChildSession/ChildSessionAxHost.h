#pragma once

#include <Windows.h>
#include <Unknwn.h>

BOOL
WINAPI
ChildSession_AxHostInitialize(VOID);

PCWSTR
WINAPI
ChildSession_AxHostClassName(VOID);

HRESULT
WINAPI
ChildSession_AxHostGetControl(
    _In_ HWND Window,
    _COM_Outptr_ IUnknown** Control);
