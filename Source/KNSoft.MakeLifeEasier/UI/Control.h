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
    UINT        Flags;
    UINT_PTR    Id;
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

FORCEINLINE
HMENU
UI_CreateWindowMenuEx(
    _In_reads_(Count) PUI_MENU_ITEM Items,
    _In_ UINT Count)
{
    HMENU hMenu;

    hMenu = CreateMenu();
    if (hMenu == NULL)
    {
        return NULL;
    }
    if (UI_CreateMenuItemsEx(hMenu, Items, Count) != ERROR_SUCCESS)
    {
        DestroyMenu(hMenu);
        return NULL;
    }
    return hMenu;
}

#define UI_CreateWindowMenu(Items) UI_CreateWindowMenuEx(Items, ARRAYSIZE(Items))

FORCEINLINE
VOID
UI_DestroyWindowMenuEx(
    _In_ HMENU Menu,
    _In_reads_(Count) PUI_MENU_ITEM Items,
    _In_ UINT Count)
{
    DestroyMenu(Menu);
    UI_DestroyMenuItemsEx(Items, Count);
}

#define UI_DestroyWindowMenu(Menu, Items) UI_DestroyWindowMenuEx(Menu, Items, ARRAYSIZE(Items))

// Return FALSE to stop enumeration, UI_EnumTreeViewItems will returns S_FALSE
typedef
_Function_class_(UI_ENUMTREEVIEWITEM_FN)
LOGICAL
CALLBACK
UI_ENUMTREEVIEWITEM_FN(
    _In_ HWND TreeView,
    _In_ HTREEITEM TreeItem,
    _In_ UINT Level,
    _In_opt_ PVOID Context);
typedef UI_ENUMTREEVIEWITEM_FN *PUI_ENUMTREEVIEWITEM_FN;

MLE_API
HRESULT
NTAPI
UI_EnumTreeViewItems(
    _In_ HWND TreeView,
    _In_ LOGICAL BFS,
    _In_ __callback PUI_ENUMTREEVIEWITEM_FN TreeItemEnumProc,
    _In_opt_ PVOID Context);

EXTERN_C_END
