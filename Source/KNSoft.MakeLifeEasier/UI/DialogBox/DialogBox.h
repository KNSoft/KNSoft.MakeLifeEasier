﻿#pragma once

#include "../../MakeLifeEasier.h"

EXTERN_C_START

/* Common dialog box */

FORCEINLINE
INT
UI_MsgBox(
    _In_opt_ HWND Owner,
    _In_opt_ LPCWSTR Text,
    _In_opt_ LPCWSTR Title,
    _In_ UINT Type)
{
    return MessageBoxTimeoutW(Owner, Text, Title, Type, 0, -1);
}

FORCEINLINE
_Success_(return != FALSE)
BOOL
UI_GetOpenFileNameEx(
    _In_opt_ HWND Owner,
    _In_opt_ PCWSTR Filter,
    _Inout_updates_(CchFile) PWSTR File,
    _In_ DWORD CchFile,
    _In_opt_ PCWSTR DefExt)
{
    OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };

    ofn.hwndOwner = Owner;
    ofn.lpstrFilter = Filter;
    ofn.lpstrFile = File;
    ofn.nMaxFile = CchFile;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
    ofn.lpstrDefExt = DefExt;

    return GetOpenFileNameW(&ofn);
}

#define UI_GetOpenFileName(Owner, Filter, File, DefExt) UI_GetOpenFileNameEx(Owner, Filter, File, ARRAYSIZE(File), DefExt)

FORCEINLINE
_Success_(return != FALSE)
BOOL
UI_GetSaveFileNameEx(
    _In_opt_ HWND Owner,
    _In_opt_ PCWSTR Filter,
    _Inout_updates_(CchFile) PWSTR File,
    _In_ DWORD CchFile,
    _In_opt_ PCWSTR DefExt)
{
    OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };

    ofn.hwndOwner = Owner;
    ofn.lpstrFilter = Filter;
    ofn.lpstrFile = File;
    ofn.nMaxFile = CchFile;
    ofn.Flags = OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = DefExt;

    return GetSaveFileNameW(&ofn);
}

#define UI_GetSaveFileName(Owner, Filter, File, DefExt) UI_GetSaveFileNameEx(Owner, Filter, File, ARRAYSIZE(File), DefExt)

/* MakeLifeEasier dialog box */

MLE_API
HRESULT
NTAPI
UI_RectEditorDlg(
    _In_opt_ HWND Owner,
    _Inout_ PRECT Rect);

typedef struct _UI_VALUEEDITOR_CONSTANT
{
    UINT64  Value;
    PCWSTR  Name;
    PCWSTR  Description;
} UI_VALUEEDITOR_CONSTANT, *PUI_VALUEEDITOR_CONSTANT;

typedef enum _UI_VALUEEDITOR_TYPE
{
    UIValueEditorCombine,
    UIValueEditorEnum, // TODO: Unimplemented
    UIValueEditorMax
} UI_VALUEEDITOR_TYPE, *PUI_VALUEEDITOR_TYPE;

MLE_API
HRESULT
NTAPI
UI_ValueEditorDlg(
    _In_opt_ HWND Owner,
    _In_ UI_VALUEEDITOR_TYPE Type,
    _Inout_ PVOID Value,
    _In_ ULONG ValueSize,
    _In_reads_(ConstantCount) UI_VALUEEDITOR_CONSTANT Constants[],
    _In_ ULONG ConstantCount);

EXTERN_C_END
