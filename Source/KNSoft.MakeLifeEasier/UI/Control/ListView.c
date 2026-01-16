#include "../../MakeLifeEasier.inl"

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
