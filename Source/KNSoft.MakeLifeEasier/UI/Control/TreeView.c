#include "../../MakeLifeEasier.inl"

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
