#pragma once

#include "../../MakeLifeEasier.h"

EXTERN_C_START

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
