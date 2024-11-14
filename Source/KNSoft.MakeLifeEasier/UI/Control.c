#include "../MakeLifeEasier.inl"

VOID
NTAPI
UI_SetDialogFont(
    _In_ HWND Dialog,
    _In_opt_ HFONT Font)
{
    HWND hWnd = GetWindow(Dialog, GW_CHILD);
    while (hWnd)
    {
        UI_SetWindowFont(hWnd, Font, FALSE);
        hWnd = GetWindow(hWnd, GW_HWNDNEXT);
    }
    UI_Redraw(Dialog);
}

LOGICAL
NTAPI
UI_ListViewSort(
    _In_ HWND List,
    _In_ PFNLVCOMPARE CompareFn,
    _In_ INT ColumnIndex)
{
    HWND hHeader;
    INT i;
    UINT Flag;
    HDITEMW stHDI;

    hHeader = ListView_GetHeader(List);
    stHDI.mask = HDI_FORMAT;
    if (!Header_GetItem(hHeader, ColumnIndex, &stHDI))
    {
        return FALSE;
    }
    Flag = FlagOn(stHDI.fmt, HDF_SORTUP) ? HDF_SORTDOWN : HDF_SORTUP;
    ClearFlag(stHDI.fmt, HDF_SORTDOWN | HDF_SORTUP);
    SetFlag(stHDI.fmt, Flag);
    if (!Header_SetItem(hHeader, ColumnIndex, &stHDI))
    {
        return FALSE;
    }

    i = Header_GetItemCount(hHeader);
    while (--i >= 0)
    {
        if (Header_GetItem(hHeader, i, &stHDI))
        {
            if (i != ColumnIndex)
            {
                ClearFlag(stHDI.fmt, HDF_SORTDOWN | HDF_SORTUP);
            }
            Header_SetItem(hHeader, i, &stHDI);
        }
    }

    if (Flag == HDF_SORTDOWN)
    {
        if (ColumnIndex != 0)
        {
            ColumnIndex = -ColumnIndex;
        } else
        {
            ColumnIndex = INT_MIN;
        }
    }
    return ListView_SortItems(List, CompareFn, ColumnIndex);
}

W32ERROR
NTAPI
UI_CreateMenuItemsEx(
    _In_ HMENU Parent,
    _Inout_updates_(Count) PUI_MENU_ITEM Items,
    _In_ UINT Count)
{
    W32ERROR eRet;
    UINT i, j;
    UINT_PTR uIDNewItem;

    for (i = 0; i < Count; i++)
    {
        if (Items[i].SubMenusCount > 0)
        {
            Items[i].Handle = CreatePopupMenu();
            if (Items[i].Handle == NULL)
            {
                break;
            }
            eRet = UI_CreateMenuItemsEx(Items[i].Handle, Items[i].SubMenus, Items[i].SubMenusCount);
            if (eRet != ERROR_SUCCESS)
            {
                NtSetLastError(eRet);
                break;
            }
            Items[i].Flags |= MF_POPUP;
            uIDNewItem = (UINT_PTR)Items[i].Handle;
        } else
        {
            Items[i].Handle = NULL;
            uIDNewItem = Items[i].Id;
        }
        if (!AppendMenuW(Parent, Items[i].Flags, uIDNewItem, Items[i].Text))
        {
            break;
        }
    }

    if (i < Count)
    {
        eRet = NtGetLastError();
        for (j = 0; j <= i; j++)
        {
            if (Items[j].Handle != NULL)
            {
                DestroyMenu(Items[i].Handle);
            }
            UI_DestroyMenuItemsEx(Items[j].SubMenus, Items[j].SubMenusCount);
        }
        return eRet;
    }

    return ERROR_SUCCESS;
}
