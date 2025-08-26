#pragma once

#include "../../MakeLifeEasier.h"

EXTERN_C_START

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
    _Inout_updates_(Count) UI_MENU_ITEM Items[],
    _In_ UINT Count);

#define UI_CreateMenuItems(Parent, Items) UI_CreateMenuItemsEx(Parent, Items, ARRAYSIZE(Items))

FORCEINLINE
W32ERROR
UI_CreateMenuEx(
    _Out_ HMENU* MenuHandle,
    _In_ LOGICAL Popup,
    _Inout_updates_(Count) UI_MENU_ITEM Items[],
    _In_ UINT Count)
{
    W32ERROR Ret;
    HMENU hMenu = Popup ? CreatePopupMenu() : CreateMenu();

    if (hMenu == NULL)
    {
        return Err_GetLastError();
    }
    Ret = UI_CreateMenuItemsEx(hMenu, Items, Count);
    if (Ret == ERROR_SUCCESS)
    {
        *MenuHandle = hMenu;
    } else
    {
        DestroyMenu(hMenu);
    }
    return Ret;
}

#define UI_CreateMenu(MenuHandle, Popup, Items) UI_CreateMenuEx(MenuHandle, Popup, Items, ARRAYSIZE(Items))

FORCEINLINE
VOID
UI_DestroyMenuItemsEx(
    _In_reads_(Count) UI_MENU_ITEM Items[],
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

/* Return current check state (S_OK: checked, S_FALSE: Unchecked) */
FORCEINLINE
HRESULT
UI_ToggleMenuCheckItem(
    _In_ HMENU Menu,
    _In_ UINT Item,
    _In_ BOOL ByPosition)
{
    MENUITEMINFOW mii;

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STATE;
    if (!GetMenuItemInfoW(Menu, Item, ByPosition, &mii))
    {
        return HRESULT_FROM_WIN32(Err_GetLastError());
    }

    mii.fState ^= MFS_CHECKED;
    if (!SetMenuItemInfoW(Menu, Item, ByPosition, &mii))
    {
        return HRESULT_FROM_WIN32(Err_GetLastError());
    }

    return mii.fState & MFS_CHECKED ? S_OK : S_FALSE;
}

FORCEINLINE
BOOL
UI_EnableMenuItem(
    _In_ HMENU Menu,
    _In_ UINT Item,
    _In_ BOOL ByPosition,
    _In_ BOOL Enable)
{
    MENUITEMINFOW mii;

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STATE;
    mii.fState = Enable ? MFS_ENABLED : MFS_DISABLED;
    return SetMenuItemInfoW(Menu, Item, ByPosition, &mii);
}

EXTERN_C_END
