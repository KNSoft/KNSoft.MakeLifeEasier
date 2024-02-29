#include "Test.h"

int wmain(int argc, wchar_t** argv)
{
    BOOL bRet = TRUE;

    /* Run tests */
    bRet &= Test_new_delete();
    bRet &= Test_Memory_Allocate();
    bRet &= Test_String();

    /* Run samples */
    bRet &= Sample_ListFile();
    bRet &= Sample_QueryStorageProperty();
    bRet &= Sample_PrintFirmwareTable();
    
    return bRet ? 0 : 1;
}
