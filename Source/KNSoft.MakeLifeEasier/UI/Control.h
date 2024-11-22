#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

MLE_API
VOID
NTAPI
UI_SetDialogFont(
    _In_ HWND Dialog,
    _In_opt_ HFONT Font);

MLE_API
LOGICAL
NTAPI
UI_ListViewSort(
    _In_ HWND List,
    _In_ PFNLVCOMPARE CompareFn,
    _In_ INT ColumnIndex);

typedef struct _UI_MENU_ITEM UI_MENU_ITEM, *PUI_MENU_ITEM;
struct _UI_MENU_ITEM
{
    BOOL        Invalid;
    UINT        Flags;      // MF_*
    UINT_PTR    Id;
    HBITMAP     Icon;
    union
    {
        LPARAM      Param;  // MF_OWNERDRAW
        HBITMAP     Bitmap; // MF_BITMAP
        PCWSTR      Text;   // MF_STRING
    };
    HMENU           Handle; // Initializes with NULL
    UINT            SubMenusCount;
    PUI_MENU_ITEM   SubMenus;
};

MLE_API
W32ERROR
NTAPI
UI_CreateMenuItemsEx(
    _In_ HMENU Parent,
    _Inout_updates_(Count) PUI_MENU_ITEM Items,
    _In_ UINT Count);

#define UI_CreateMenuItems(Parent, Items) UI_CreateMenuItemsEx(Parent, Items, ARRAYSIZE(Items))

FORCEINLINE
VOID
UI_DestroyMenuItemsEx(
    _In_reads_(Count) PUI_MENU_ITEM Items,
    _In_ UINT Count)
{
    UINT i;

    for (i = 0; i < Count; i++)
    {
        if (Items[i].Handle != NULL)
        {
            DestroyMenu(Items[i].Handle);
        }
        if (Items[i].SubMenusCount > 0)
        {
            UI_DestroyMenuItemsEx(Items[i].SubMenus, Items[i].SubMenusCount);
        }
    }
}

#define UI_DestroyMenuItems(Items) UI_DestroyMenuItemsEx(Items, ARRAYSIZE(Items))

/// <summary>
/// Popups a menu
/// </summary>
/// <seealso cref="TrackPopupMenu"/>
FORCEINLINE
BOOL
UI_PopupMenu(
    _In_ HMENU Menu,
    _In_ INT X,
    _In_ INT Y,
    _In_ HWND Window)
{
    return TrackPopupMenu(Menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, X, Y, 0, Window, NULL);
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

MLE_API
HRESULT
NTAPI
UI_TreeViewEnumItems(
    _In_ HWND TreeView,
    _In_ LOGICAL BFS,
    _In_ __callback PUI_TREEVIEW_ENUMITEM_FN TreeItemEnumProc,
    _In_opt_ PVOID Context);

EXTERN_C_END
