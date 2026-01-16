#include "../../MakeLifeEasier.inl"

W32ERROR
NTAPI
UI_CreateMenuItemsEx(
    _In_ HMENU Parent,
    _Inout_updates_(Count) UI_MENU_ITEM Items[],
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
                Err_SetLastError(eRet);
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
        if (Items[i].Flags & MF_DEFAULT)
        {
            SetMenuDefaultItem(Parent, Index, MF_BYPOSITION);
        }
        Index++;
    }

    if (i < Count)
    {
        eRet = Err_GetLastError();
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
