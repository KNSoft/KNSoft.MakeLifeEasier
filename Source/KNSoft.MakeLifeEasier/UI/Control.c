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
    UINT i, j, Index;
    UINT_PTR uIDNewItem;

    Index = 0;
    for (i = 0; i < Count; i++)
    {
        if (Items[i].Invalid)
        {
            continue;
        }
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
        if (Items[i].Icon != NULL)
        {
            SetMenuItemBitmaps(Parent, Index, MF_BYPOSITION, Items[i].Icon, Items[i].Icon);
        }
        Index++;
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

HRESULT
NTAPI
UI_TreeViewEnumItems(
    _In_ HWND TreeView,
    _In_ LOGICAL BFS,
    _In_ __callback PUI_TREEVIEW_ENUMITEM_FN TreeItemEnumProc,
    _In_opt_ PVOID Context)
{
    UINT uDepth;
    HTREEITEM hItem, hItemTemp;

    if (BFS)
    {
        // TODO: Implement BFS
        return E_NOTIMPL;
    }

    uDepth = 0;
    hItem = (HTREEITEM)SendMessageW(TreeView, TVM_GETNEXTITEM, TVGN_ROOT, 0);
    do
    {
        if (!TreeItemEnumProc(TreeView, hItem, uDepth, Context))
        {
            return S_FALSE;
        }

        hItemTemp = hItem;
        hItem = (HTREEITEM)SendMessageW(TreeView, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)hItemTemp);
        if (hItem != NULL)
        {
            uDepth++;
            continue;
        }
        hItem = (HTREEITEM)SendMessageW(TreeView, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hItemTemp);
        if (hItem != NULL)
        {
            continue;
        }
        while (--uDepth)
        {
            hItem = (HTREEITEM)SendMessageW(TreeView, TVM_GETNEXTITEM, TVGN_PARENT, (LPARAM)hItemTemp);
            hItemTemp = (HTREEITEM)SendMessageW(TreeView, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hItem);
            if (hItemTemp != NULL)
            {
                hItem = hItemTemp;
                break;
            } else
            {
                hItemTemp = hItem;
            }
        }
    } while (uDepth != 0);

    return S_OK;
}
