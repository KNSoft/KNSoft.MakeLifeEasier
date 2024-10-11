#include "../../MakeLifeEasier.inl"

#define MAX_UINT64_IN_HEX_WITH_PREFIX_CCH (17 + 2) // 0x prefix

typedef struct _UI_VALUEEDITOR_DATA
{
    ULONG                       Type;
    PVOID                       Value;
    ULONG                       ValueSize;
    PUI_VALUEEDITOR_CONSTANT    Constants;
    ULONG                       ConstantCount;
    W32ERROR                    Ret;

    /* Internal use */
    UINT64                      CurrentValue;
} UI_VALUEEDITOR_DATA, *PUI_VALUEEDITOR_DATA;

static
UINT64
UI_ValueEditor_GetValue(
    _In_ PVOID Value,
    _In_ ULONG ValueSize)
{
    if (ValueSize == sizeof(UINT64))
    {
        return *(PUINT64)Value;
    } else if (ValueSize == sizeof(UINT32))
    {
        return *(PUINT32)Value;
    } else if (ValueSize == sizeof(UINT16))
    {
        return *(PUINT16)Value;
    } else
    {
        return *(PUINT8)Value;
    }
}

static
VOID
UI_ValueEditor_SetValue(
    _In_ PVOID Value,
    _In_ ULONG ValueSize,
    _In_ UINT64 NewValue)
{
    if (ValueSize == sizeof(UINT64))
    {
        *(PUINT64)Value = NewValue;
    } else if (ValueSize == sizeof(UINT32))
    {
        *(PUINT32)Value = (UINT32)NewValue;
    } else if (ValueSize == sizeof(UINT16))
    {
        *(PUINT16)Value = (UINT16)NewValue;
    } else
    {
        *(PUINT8)Value = (UINT8)NewValue;
    }
}

_Success_(
    return > 0 && return < BufferCount
)
static
LOGICAL
UI_ValueEditor_FormatValue(
    _Out_writes_(BufferCount) PWSTR Buffer,
    _In_ SIZE_T BufferCount,
    _In_ PVOID Value,
    _In_ ULONG ValueSize)
{
    ULONG i;

    if (ValueSize == sizeof(UINT64))
    {
        i = Str_PrintfExW(Buffer, BufferCount, L"0x%016llX", *(PUINT64)Value);
    } else if (ValueSize == sizeof(UINT32))
    {
        i = Str_PrintfExW(Buffer, BufferCount, L"0x%08lX", *(PUINT32)Value);
    } else if (ValueSize == sizeof(UINT16))
    {
        i = Str_PrintfExW(Buffer, BufferCount, L"0x%04hX", *(PUINT16)Value);
    } else if (ValueSize == sizeof(UINT8))
    {
        i = Str_PrintfExW(Buffer, BufferCount, L"0x%02hhX", *(PUINT8)Value);
    } else
    {
        return FALSE;
    }

    return i > 0 && i < BufferCount;
}

static
VOID
UI_ValueEditor_SetValueText(
    _In_ HWND Dialog,
    _In_ PVOID Value,
    _In_ ULONG ValueSize)
{
    WCHAR String[MAX_UINT64_IN_HEX_WITH_PREFIX_CCH];

    UI_SetDlgItemTextW(Dialog,
                       IDC_VALUEEDITOR_VALUE_EDIT,
                       UI_ValueEditor_FormatValue(String,
                                                  ARRAYSIZE(String),
                                                  Value,
                                                  ValueSize) ? String : NULL);
}

static
LOGICAL
UI_ValueEditor_AppendUnidentifiedMember(
    _In_ HWND List,
    _In_ UINT64 Value,
    _In_ ULONG ValueSize,
    _In_ UINT64 IdentifiedValue)
{
    LVITEMW stLVI;
    WCHAR String[MAX_UINT64_IN_HEX_WITH_PREFIX_CCH];

    if (Value <= IdentifiedValue)
    {
        return TRUE;
    }

    Value -= IdentifiedValue;
    stLVI.iItem = INT_MAX;
    stLVI.mask = LVIF_TEXT | LVIF_PARAM;
    stLVI.iSubItem = 0;
    stLVI.lParam = 0;
    stLVI.pszText = (PWSTR)Mlep_GetString(Precomp4C_I18N_All_Unidentified_Parentheses);
    stLVI.iItem = ListView_InsertItem(List, &stLVI);
    if (stLVI.iItem != -1)
    {
        ListView_SetCheckState(List, stLVI.iItem, TRUE);
        stLVI.iSubItem++;
        stLVI.lParam = HIDWORD(Value);
        stLVI.pszText = UI_ValueEditor_FormatValue(String,
                                                   ARRAYSIZE(String),
                                                   &Value,
                                                   ValueSize) ? String : NULL;
        ListView_SetItem(List, &stLVI);
        stLVI.mask = LVIF_PARAM;
        stLVI.iSubItem++;
        stLVI.lParam = LODWORD(Value);
        ListView_SetItem(List, &stLVI);
    }

    return TRUE;
}

_Success_(return != FALSE)
static
LOGICAL
UI_ValueEditor_UpdateValue(
    _In_ HWND Dialog,
    _In_ ULONG ValueSize,
    _In_ UINT64 ValueInput,
    _Out_opt_ PUINT64 ValueOutput)
{
    HWND hList;
    INT i, iUnidentified;
    LVITEMW stLVI;
    UINT64 qwTemp, qwUnidentified;
    BOOL bCheck;
    PUI_VALUEEDITOR_CONSTANT pConstData;

    hList = GetDlgItem(Dialog, IDC_VALUEEDITOR_MEMBER_LIST);

    qwTemp = 0;
    i = ListView_GetItemCount(hList);
    iUnidentified = -1;
    stLVI.iSubItem = 0;
    stLVI.mask = LVIF_PARAM;
    while (i-- > 0)
    {
        stLVI.iItem = i;
        if (!ListView_GetItem(hList, &stLVI))
        {
            return FALSE;
        }
        if (stLVI.lParam == 0)
        {
            iUnidentified = stLVI.iItem;
            continue;
        }

        pConstData = (PUI_VALUEEDITOR_CONSTANT)stLVI.lParam;

        if (ValueOutput == NULL)
        {
            if (ValueInput & pConstData->Value)
            {
                bCheck = TRUE;
                qwTemp |= pConstData->Value;
            } else
            {
                bCheck = FALSE;
            }
            ListView_SetCheckState(hList, stLVI.iItem, bCheck);
        } else
        {
            qwTemp |= pConstData->Value;
        }
    }

    if (ValueOutput == NULL)
    {
        if (iUnidentified != -1)
        {
            if (!ListView_DeleteItem(hList, iUnidentified))
            {
                return FALSE;
            }
        }
        if (!UI_ValueEditor_AppendUnidentifiedMember(hList, ValueInput, ValueSize, qwTemp))
        {
            return FALSE;
        }
    } else
    {
        if (iUnidentified != -1)
        {
            stLVI.iItem = iUnidentified;
            stLVI.iSubItem = 1;
            stLVI.mask = LVIF_PARAM;
            if (!ListView_GetItem(hList, &stLVI))
            {
                return FALSE;
            }
            qwUnidentified = (UINT64)stLVI.lParam & 0xFFFFFFFF;
            qwUnidentified <<= 32;
            stLVI.iSubItem++;
            if (!ListView_GetItem(hList, &stLVI))
            {
                return FALSE;
            }
            qwUnidentified &= (UINT64)stLVI.lParam & 0xFFFFFFFF;
            qwTemp += qwUnidentified;
        }
        *ValueOutput = qwTemp;
    }

    return TRUE;
}

static INT g_aColCx[] = { 200, 210, 540 };
static INT g_aColPsz[] = {
    Precomp4C_I18N_All_Member,
    Precomp4C_I18N_All_Value,
    Precomp4C_I18N_All_Description };
C_ASSERT(ARRAYSIZE(g_aColCx) == ARRAYSIZE(g_aColPsz));

static
INT_PTR
CALLBACK
DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UI_DPIScaleDlgProc(hDlg, uMsg, wParam, lParam);
    if (uMsg == WM_INITDIALOG)
    {
        PUI_VALUEEDITOR_DATA pData = (PUI_VALUEEDITOR_DATA)lParam;
        HWND hList;
        ULONG i;
        LVCOLUMNW stLVCol;
        LVITEMW stLVI;
        UINT64 qwValue, qwTemp;
        WCHAR String[MAX_UINT64_IN_HEX_WITH_PREFIX_CCH];

        UI_InitializeDialogDPIScale(hDlg);
        SetWindowLongPtrW(hDlg, DWLP_USER, lParam);
        UI_SetWindowTextW(hDlg, Mlep_GetString(pData->Type == UIValueEditorCombine ?
                                               Precomp4C_I18N_All_ValueEditorCombine :
                                               Precomp4C_I18N_All_EnumEditorCombine));

        pData->CurrentValue = UI_ValueEditor_GetValue(pData->Value, pData->ValueSize);
        UI_ValueEditor_SetValueText(hDlg, pData->Value, pData->ValueSize);

        hList = GetDlgItem(hDlg, IDC_VALUEEDITOR_MEMBER_LIST);
        ListView_SetExtendedListViewStyle(hList, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);

        UI_SetNoNotifyFlag(hList, TRUE);

        /* Add columns */
        stLVCol.mask = LVCF_TEXT | LVCF_WIDTH;
        for (i = 0; i < ARRAYSIZE(g_aColCx); i++)
        {
            stLVCol.cx = g_aColCx[i];
            stLVCol.pszText = (PWSTR)Mlep_GetString(g_aColPsz[i]);
            ListView_InsertColumn(hList, i, &stLVCol);
        }

        /* Add rows */
        stLVI.iItem = INT_MAX;
        qwValue = UI_ValueEditor_GetValue(pData->Value, pData->ValueSize);
        qwTemp = 0;
        for (i = 0; i < pData->ConstantCount; i++)
        {
            stLVI.iSubItem = 0;
            stLVI.mask = LVIF_TEXT | LVIF_PARAM;
            stLVI.lParam = (LPARAM)&pData->Constants[i];
            stLVI.pszText = (PWSTR)pData->Constants[i].Name;
            stLVI.iItem = ListView_InsertItem(hList, &stLVI);
            if (stLVI.iItem != -1)
            {
                stLVI.mask = LVIF_TEXT;
                if (TEST_FLAGS(qwValue, pData->Constants[i].Value))
                {
                    ListView_SetCheckState(hList, stLVI.iItem, TRUE);
                    qwTemp |= pData->Constants[i].Value;
                }
                stLVI.iSubItem++;
                stLVI.pszText = UI_ValueEditor_FormatValue(String,
                                                           ARRAYSIZE(String),
                                                           &pData->Constants[i].Value,
                                                           pData->ValueSize) ? String : NULL;
                ListView_SetItem(hList, &stLVI);
                stLVI.iSubItem++;
                stLVI.pszText = (LPWSTR)pData->Constants[i].Info;
                ListView_SetItem(hList, &stLVI);
            }
            stLVI.iItem++;
        }
        UI_ValueEditor_AppendUnidentifiedMember(hList, qwValue, pData->ValueSize, qwTemp);

        UI_SetNoNotifyFlag(hList, FALSE);

        UI_SetDlgItemTextW(hDlg, IDOK, Mlep_GetString(Precomp4C_I18N_All_OK));
        UI_SetDlgItemTextW(hDlg, IDRETRY, Mlep_GetString(Precomp4C_I18N_All_Reset));

        return FALSE;
    } else if (uMsg == WM_NOTIFY)
    {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        if (lpnm->idFrom == IDC_VALUEEDITOR_MEMBER_LIST)
        {
            LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)lParam;

            if (lpnmlv->hdr.code == LVN_ITEMCHANGED &&
                lpnmlv->iItem != -1 &&
                lpnmlv->uChanged == LVIF_STATE &&
                !UI_GetNoNotifyFlag(lpnmlv->hdr.hwndFrom))
            {
                PUI_VALUEEDITOR_DATA pData = (PUI_VALUEEDITOR_DATA)GetWindowLongPtrW(hDlg, DWLP_USER);
                UINT64 qwValue;

                UI_SetNoNotifyFlag(lpnmlv->hdr.hwndFrom, TRUE);
                UI_ValueEditor_UpdateValue(hDlg, pData->ValueSize, 0, &qwValue);
                UI_SetNoNotifyFlag(lpnmlv->hdr.hwndFrom, FALSE);
                pData->CurrentValue = qwValue;
                UI_ValueEditor_SetValueText(hDlg, &qwValue, pData->ValueSize);
            }
        }
    } else if (uMsg == WM_COMMAND)
    {
        PUI_VALUEEDITOR_DATA pData = (PUI_VALUEEDITOR_DATA)GetWindowLongPtrW(hDlg, DWLP_USER);

        if (wParam == MAKEWPARAM(IDRETRY, 0))
        {
            pData->CurrentValue = UI_ValueEditor_GetValue(pData->Value, pData->ValueSize);
            UI_ValueEditor_SetValueText(hDlg, pData->Value, pData->ValueSize);
            UI_SetNoNotifyFlag((HWND)lParam, TRUE);
            UI_ValueEditor_UpdateValue(hDlg, pData->ValueSize, pData->CurrentValue, NULL);
            UI_SetNoNotifyFlag((HWND)lParam, FALSE);
        } else if (wParam == MAKEWPARAM(IDOK, 0))
        {
            UI_ValueEditor_SetValue(pData->Value, pData->ValueSize, pData->CurrentValue);
            EndDialog(hDlg, TRUE);
        }

        SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, 0);
    } else if (uMsg == WM_CLOSE)
    {
        PUI_VALUEEDITOR_DATA pData = (PUI_VALUEEDITOR_DATA)GetWindowLongPtrW(hDlg, DWLP_USER);

        pData->Ret = ERROR_CANCELLED;
        EndDialog(hDlg, 0);
        SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, 0);
    } else if (uMsg == WM_DESTROY)
    {
        UI_UninitializeDialogDPIScale(hDlg);
    }
    return 0;
}

W32ERROR
NTAPI
UI_ValueEditorDlg(
    _In_opt_ HWND Owner,
    _In_ UI_VALUEEDITOR_TYPE Type,
    _Inout_ PVOID Value,
    _In_ ULONG ValueSize,
    _In_reads_(ConstantCount) UI_VALUEEDITOR_CONSTANT Constants[],
    _In_ ULONG ConstantCount)
{
    W32ERROR Ret;

    if (Type == UIValueEditorEnum)
    {
        return ERROR_NOT_SUPPORTED;
    } else if (Type >= UIValueEditorMax)
    {
        return ERROR_INVALID_PARAMETER;
    }

    UI_VALUEEDITOR_DATA Data = {
        .Type = Type,
        .Value = Value,
        .ValueSize = ValueSize,
        .Constants = Constants,
        .ConstantCount = ConstantCount,
        .Ret = ERROR_INTERNAL_ERROR
    };

    Ret = Mlep_DlgBox(MAKEINTRESOURCEW(IDD_VALUEEDITOR), Owner, DlgProc, (LPARAM)&Data, NULL);
    return Ret == ERROR_SUCCESS ? Data.Ret : Ret;
}
