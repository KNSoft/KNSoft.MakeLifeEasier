#include "../../MakeLifeEasier.inl"

#define MAX_UINT64_IN_HEX_WITH_PREFIX_CCH (17 + 2) // 0x prefix

typedef struct _UI_VALUEEDITOR_DATA
{
    ULONG                       Type;
    PVOID                       Value;
    ULONG                       ValueSize;
    PUI_VALUEEDITOR_CONSTANT    Constants;
    ULONG                       ConstantCount;

    /* Internal use */
    UINT64                      CurrentValue;
    UINT64                      UnidentifiedValue;
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

    return i > 0;
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
    _Inout_ PUI_VALUEEDITOR_DATA Data,
    _In_ UINT64 IdentifiedValue)
{
    LVITEMW stLVI;
    WCHAR String[MAX_UINT64_IN_HEX_WITH_PREFIX_CCH];

    if (Data->CurrentValue <= IdentifiedValue)
    {
        Data->UnidentifiedValue = 0;
        return TRUE;
    }

    Data->UnidentifiedValue = Data->CurrentValue - IdentifiedValue;
    stLVI.iItem = INT_MAX;
    stLVI.mask = LVIF_TEXT | LVIF_PARAM;
    stLVI.iSubItem = 0;
    stLVI.lParam = 0;
    stLVI.pszText = (PWSTR)Mlep_GetString(Unidentified_Parentheses);
    stLVI.iItem = ListView_InsertItem(List, &stLVI);
    if (stLVI.iItem != -1)
    {
        stLVI.mask = LVIF_TEXT;
        ListView_SetCheckState(List, stLVI.iItem, TRUE);
        stLVI.iSubItem++;
        stLVI.pszText = UI_ValueEditor_FormatValue(String,
                                                   ARRAYSIZE(String),
                                                   &Data->UnidentifiedValue,
                                                   Data->ValueSize) ? String : NULL;
        ListView_SetItem(List, &stLVI);
    }

    return TRUE;
}

_Success_(return != FALSE)
static
LOGICAL
UI_ValueEditor_UpdateListOrValue(
    _In_ HWND List,
    _Inout_ PUI_VALUEEDITOR_DATA Data,
    _In_ BOOL UpdateList)
{
    INT i, iUnidentified;
    LVITEMW stLVI;
    UINT64 qwTemp;
    BOOL bCheck;
    PUI_VALUEEDITOR_CONSTANT pConstData;

    qwTemp = 0;
    i = ListView_GetItemCount(List);
    iUnidentified = -1;
    stLVI.iSubItem = 0;
    stLVI.mask = LVIF_PARAM;
    while (i-- > 0)
    {
        stLVI.iItem = i;
        if (!ListView_GetItem(List, &stLVI))
        {
            return FALSE;
        }
        if (stLVI.lParam == 0)
        {
            iUnidentified = stLVI.iItem;
            continue;
        }

        pConstData = (PUI_VALUEEDITOR_CONSTANT)stLVI.lParam;

        if (UpdateList)
        {
            if (Data->CurrentValue & pConstData->Value)
            {
                bCheck = TRUE;
                qwTemp |= pConstData->Value;
            } else
            {
                bCheck = FALSE;
            }
            ListView_SetCheckState(List, stLVI.iItem, bCheck);
        } else
        {
            if (ListView_GetCheckState(List, stLVI.iItem))
            {
                qwTemp |= pConstData->Value;
            }
        }
    }

    if (UpdateList)
    {
        if (iUnidentified != -1)
        {
            if (!ListView_DeleteItem(List, iUnidentified))
            {
                return FALSE;
            }
        }
        if (!UI_ValueEditor_AppendUnidentifiedMember(List, Data, qwTemp))
        {
            return FALSE;
        }
    } else
    {
        if (iUnidentified != -1 && ListView_GetCheckState(List, iUnidentified))
        {
            qwTemp += Data->UnidentifiedValue;
        }
        Data->CurrentValue = qwTemp;
    }

    return TRUE;
}

static INT g_aColCx[] = { 200, 210, 540 };
static INT g_aColPsz[] = {
    Precomp4C_I18N_All_Member,
    Precomp4C_I18N_All_Value,
    Precomp4C_I18N_All_Description
};
_STATIC_ASSERT(ARRAYSIZE(g_aColCx) == ARRAYSIZE(g_aColPsz));

/*
 * @remarks
 * iColumn >= 0 means ascend, value is the column index.
 * iColumn < 0 means descend, -value is the column index, INT_MIN means 0.
 */
static
int
CALLBACK
UI_ValueEditor_ColumnSort(
    LPARAM lParam1,
    LPARAM lParam2,
    LPARAM iColumn)
{
    PUI_VALUEEDITOR_CONSTANT pConstData1 = (PUI_VALUEEDITOR_CONSTANT)lParam1;
    PUI_VALUEEDITOR_CONSTANT pConstData2 = (PUI_VALUEEDITOR_CONSTANT)lParam2;
    BOOL Ascend = iColumn >= 0;
    INT iCmp;

    if (iColumn < 0)
    {
        if (iColumn != INT_MIN)
        {
            iColumn = -iColumn;
        } else
        {
            iColumn = 0;
        }
    }

    if (pConstData1 == pConstData2)
    {
        return 0;
    } else if (pConstData1 == NULL)
    {
        iCmp = 1;
    } else if (pConstData2 == NULL)
    {
        iCmp = -1;
    } else
    {
        if (iColumn == 0)
        {
            iCmp = wcscmp(pConstData1->Name, pConstData2->Name);
        } else if (iColumn == 1)
        {
            if (pConstData1->Value < pConstData2->Value)
            {
                iCmp = -1;
            } else if (pConstData1->Value == pConstData2->Value)
            {
                return 0;
            } else
            {
                iCmp = 1;
            }
        } else if (iColumn == 2)
        {
            iCmp = wcscmp(pConstData1->Description, pConstData2->Description);
        } else
        {
            return 0;
        }
        if (!Ascend)
        {
            iCmp = -iCmp;
        }
    }

    return iCmp;
}

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
        UINT64 qwTemp;
        WCHAR String[MAX_UINT64_IN_HEX_WITH_PREFIX_CCH];

        SetWindowLongPtrW(hDlg, DWLP_USER, lParam);
        UI_SetWindowTextW(hDlg, Mlep_GetStringEx(pData->Type == UIValueEditorCombine ?
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
            stLVCol.pszText = (PWSTR)Mlep_GetStringEx(g_aColPsz[i]);
            ListView_InsertColumn(hList, i, &stLVCol);
        }

        /* Add rows */
        stLVI.iItem = INT_MAX;
        pData->CurrentValue = UI_ValueEditor_GetValue(pData->Value, pData->ValueSize);
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
                if (TEST_FLAGS(pData->CurrentValue, pData->Constants[i].Value))
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
                stLVI.pszText = (PWSTR)pData->Constants[i].Description;
                ListView_SetItem(hList, &stLVI);
            }
            stLVI.iItem++;
        }
        UI_ValueEditor_AppendUnidentifiedMember(hList, pData, qwTemp);

        UI_SetNoNotifyFlag(hList, FALSE);

        UI_SetDlgItemTextW(hDlg, IDOK, Mlep_GetString(OK));
        UI_SetDlgItemTextW(hDlg, IDRETRY, Mlep_GetString(Reset));

        return TRUE;
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

                UI_SetNoNotifyFlag(lpnmlv->hdr.hwndFrom, TRUE);
                UI_ValueEditor_UpdateListOrValue(lpnmlv->hdr.hwndFrom, pData, FALSE);
                UI_SetNoNotifyFlag(lpnmlv->hdr.hwndFrom, FALSE);
                UI_ValueEditor_SetValueText(hDlg, &pData->CurrentValue, pData->ValueSize);
            } else if (lpnmlv->hdr.code == LVN_COLUMNCLICK && lpnmlv->iItem == -1)
            {
                UI_ListViewSort(lpnmlv->hdr.hwndFrom, UI_ValueEditor_ColumnSort, lpnmlv->iSubItem);
            }
        }
    } else if (uMsg == WM_COMMAND)
    {
        PUI_VALUEEDITOR_DATA pData = (PUI_VALUEEDITOR_DATA)GetWindowLongPtrW(hDlg, DWLP_USER);

        if (wParam == MAKEWPARAM(IDRETRY, 0))
        {
            HWND hList = GetDlgItem(hDlg, IDC_VALUEEDITOR_MEMBER_LIST);

            pData->CurrentValue = UI_ValueEditor_GetValue(pData->Value, pData->ValueSize);
            UI_ValueEditor_SetValueText(hDlg, pData->Value, pData->ValueSize);
            UI_SetNoNotifyFlag(hList, TRUE);
            UI_ValueEditor_UpdateListOrValue(hList, pData, TRUE);
            UI_SetNoNotifyFlag(hList, FALSE);
        } else if (wParam == MAKEWPARAM(IDOK, 0))
        {
            UI_ValueEditor_SetValue(pData->Value, pData->ValueSize, pData->CurrentValue);
            EndDialog(hDlg, S_OK);
        }

        SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, 0);
    } else if (uMsg == WM_CLOSE)
    {
        PUI_VALUEEDITOR_DATA pData = (PUI_VALUEEDITOR_DATA)GetWindowLongPtrW(hDlg, DWLP_USER);

        EndDialog(hDlg, S_FALSE);
        SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, 0);
    }
    return 0;
}

HRESULT
NTAPI
UI_ValueEditorDlg(
    _In_opt_ HWND Owner,
    _In_ UI_VALUEEDITOR_TYPE Type,
    _Inout_updates_bytes_(ValueSize) PVOID Value,
    _In_ ULONG ValueSize,
    _In_reads_(ConstantCount) UI_VALUEEDITOR_CONSTANT Constants[],
    _In_ ULONG ConstantCount)
{
    if (Type == UIValueEditorEnum)
    {
        return E_NOTIMPL;
    } else if (Type >= UIValueEditorMax)
    {
        return E_INVALIDARG;
    }

    UI_VALUEEDITOR_DATA Data = {
        .Type = Type,
        .Value = Value,
        .ValueSize = ValueSize,
        .Constants = Constants,
        .ConstantCount = ConstantCount
    };

    return Mlep_DlgBox(Owner, MAKEINTRESOURCEW(IDD_VALUEEDITOR), DlgProc, (LPARAM)&Data);
}
