#pragma once

#include "../../MakeLifeEasier.h"

EXTERN_C_START

FORCEINLINE
_Success_(return != NULL)
HTREEITEM
_Ret_maybenull_
UI_TreeViewLocateItem(
    _In_ HWND TreeView,
    _In_ LONG X,
    _In_ LONG Y,
    _Out_opt_ PUINT Flags)
{
    TVHITTESTINFO tvhti;
    HTREEITEM Item;

    tvhti.pt.x = X;
    tvhti.pt.y = Y;
    Item = (HTREEITEM)SendMessageW(TreeView, TVM_HITTEST, 0, (LPARAM)&tvhti);
    if (Item != NULL && Flags != NULL)
    {
        *Flags = tvhti.flags;
    }
    return Item;
}

FORCEINLINE
_Success_(return != FALSE)
LOGICAL
UI_TreeViewGetItemParam(
    _In_ HWND TreeView,
    _In_ HTREEITEM Item,
    _Out_ LPARAM* Param)
{
    TVITEMW tvi;

    tvi.mask = TVIF_PARAM;
    tvi.hItem = Item;

    if (SendMessageW(TreeView, TVM_GETITEMW, 0, (LPARAM)&tvi))
    {
        *Param = tvi.lParam;
        return TRUE;
    }
    return FALSE;
}

// Return FALSE to stop enumeration, UI_EnumTreeViewItems will returns S_FALSE
typedef
_Function_class_(UI_TREEVIEW_ENUMITEM_FN)
LOGICAL
CALLBACK
UI_TREEVIEW_ENUMITEM_FN(
    _In_ HWND TreeView,
    _In_ HTREEITEM TreeItem,
    _In_ UINT Level,
    _In_opt_ PVOID Context);
typedef UI_TREEVIEW_ENUMITEM_FN *PUI_TREEVIEW_ENUMITEM_FN;

/* BFS (Breadth-First-Search) must be FALSE (unimplemented), support DFS (Depth-First-Search) only */
MLE_API
HRESULT
NTAPI
UI_TreeViewEnumItems(
    _In_ HWND TreeView,
    _In_ LOGICAL BFS,
    _In_ __callback PUI_TREEVIEW_ENUMITEM_FN TreeItemEnumProc,
    _In_opt_ PVOID Context);

EXTERN_C_END
