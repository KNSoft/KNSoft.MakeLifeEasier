#include "../Test.h"

#include <stdlib.h>

#define TEST_STRING1_SHORTER "Wintexports CRT String Test1"
#define TEST_STRING1_LESS "Wintexports CRT String Test10"
#define TEST_STRING1 "Wintexports CRT String Test11"
#define TEST_STRING1_GREATER "Wintexports CRT String Test12"
#define TEST_STRING1_LONGER "Wintexports CRT String Test111"
#define TEST_STRING1_CASE_MIX "winTexPorTs cRt strInG TesT11"
#define TEST_STRING1_PART1 "Wintexports C"
#define TEST_STRING1_PART2 "RT String Test11"

_STATIC_ASSERT(_countof(TEST_STRING1) == _countof(TEST_STRING1_LESS) &&
               _countof(TEST_STRING1) == _countof(TEST_STRING1_GREATER) &&
               _countof(TEST_STRING1) == _countof(TEST_STRING1_CASE_MIX) &&
               _countof(TEST_STRING1_SHORTER) < _countof(TEST_STRING1) &&
               _countof(TEST_STRING1_LONGER) > _countof(TEST_STRING1) &&
               _countof(TEST_STRING1_PART1) + _countof(TEST_STRING1_PART2) == _countof(TEST_STRING1) + 1);

BOOL Test_String()
{
    BOOL bRet = TRUE;
    char szTempA[_countof(TEST_STRING1)];
    wchar_t szTempW[_countof(TEST_STRING1)];

    SetNtdllCRTErrorMode();

    /* String Length */

    if (strlen(TEST_STRING1) != _STR_CCH_LEN(TEST_STRING1))
    {
        PrintF("Test strlen failed\n");
        bRet = FALSE;
    }
    if (wcslen(_A2W(TEST_STRING1)) != _STR_CCH_LEN(TEST_STRING1))
    {
        PrintF("Test wcslen failed\n");
        bRet = FALSE;
    }
    if (strnlen(TEST_STRING1, _countof(TEST_STRING1)) != _STR_CCH_LEN(TEST_STRING1) ||
        strnlen(TEST_STRING1, _STR_CCH_LEN(TEST_STRING1)) != _STR_CCH_LEN(TEST_STRING1))
    {
        PrintF("Test strnlen failed\n");
        bRet = FALSE;
    }
    if (wcsnlen(_A2W(TEST_STRING1), _countof(TEST_STRING1)) != _STR_CCH_LEN(TEST_STRING1) ||
        wcsnlen(_A2W(TEST_STRING1), _STR_CCH_LEN(TEST_STRING1)) != _STR_CCH_LEN(TEST_STRING1))
    {
        PrintF("Test wcsnlen failed\n");
        bRet = FALSE;
    }

    /* String Compare */

    if (strcmp(TEST_STRING1, TEST_STRING1) != 0 ||
        strcmp(TEST_STRING1_SHORTER, TEST_STRING1) >= 0 ||
        strcmp(TEST_STRING1_LESS, TEST_STRING1) >= 0 ||
        strcmp(TEST_STRING1_LONGER, TEST_STRING1) <= 0 ||
        strcmp(TEST_STRING1_GREATER, TEST_STRING1) <= 0)
    {
        PrintF("Test strcmp failed\n");
        bRet = FALSE;
    }
    if (wcscmp(_A2W(TEST_STRING1), _A2W(TEST_STRING1)) != 0 ||
        wcscmp(_A2W(TEST_STRING1_SHORTER), _A2W(TEST_STRING1)) >= 0 ||
        wcscmp(_A2W(TEST_STRING1_LESS), _A2W(TEST_STRING1)) >= 0 ||
        wcscmp(_A2W(TEST_STRING1_LONGER), _A2W(TEST_STRING1)) <= 0 ||
        wcscmp(_A2W(TEST_STRING1_GREATER), _A2W(TEST_STRING1)) <= 0)
    {
        PrintF("Test wcscmp failed\n");
        bRet = FALSE;
    }
    if (strncmp(TEST_STRING1, TEST_STRING1, _STR_CCH_LEN(TEST_STRING1)) != 0 ||
        strncmp(TEST_STRING1_SHORTER, TEST_STRING1, _STR_CCH_LEN(TEST_STRING1_SHORTER)) != 0 ||
        strncmp(TEST_STRING1_SHORTER, TEST_STRING1, _STR_CCH_LEN(TEST_STRING1)) >= 0 ||
        strncmp(TEST_STRING1, TEST_STRING1_SHORTER, _STR_CCH_LEN(TEST_STRING1)) <= 0 ||
        strncmp(TEST_STRING1_LESS, TEST_STRING1, _STR_CCH_LEN(TEST_STRING1)) >= 0 ||
        strncmp(TEST_STRING1_GREATER, TEST_STRING1, _STR_CCH_LEN(TEST_STRING1)) <= 0)
    {
        PrintF("Test strncmp failed\n");
        bRet = FALSE;
    }
    if (wcsncmp(_A2W(TEST_STRING1), _A2W(TEST_STRING1), _STR_CCH_LEN(TEST_STRING1)) != 0 ||
        wcsncmp(_A2W(TEST_STRING1_SHORTER), _A2W(TEST_STRING1), _STR_CCH_LEN(TEST_STRING1_SHORTER)) != 0 ||
        wcsncmp(_A2W(TEST_STRING1_SHORTER), _A2W(TEST_STRING1), _STR_CCH_LEN(TEST_STRING1)) >= 0 ||
        wcsncmp(_A2W(TEST_STRING1), _A2W(TEST_STRING1_SHORTER), _STR_CCH_LEN(TEST_STRING1)) <= 0 ||
        wcsncmp(_A2W(TEST_STRING1_LESS), _A2W(TEST_STRING1), _STR_CCH_LEN(TEST_STRING1)) >= 0 ||
        wcsncmp(_A2W(TEST_STRING1_GREATER), _A2W(TEST_STRING1), _STR_CCH_LEN(TEST_STRING1)) <= 0)
    {
        PrintF("Test wcsncmp failed\n");
        bRet = FALSE;
    }
    if (_stricmp(TEST_STRING1, TEST_STRING1_CASE_MIX) != 0 ||
        _stricmp(TEST_STRING1_SHORTER, TEST_STRING1_CASE_MIX) >= 0 ||
        _stricmp(TEST_STRING1_LESS, TEST_STRING1_CASE_MIX) >= 0 ||
        _stricmp(TEST_STRING1_LONGER, TEST_STRING1_CASE_MIX) <= 0 ||
        _stricmp(TEST_STRING1_GREATER, TEST_STRING1_CASE_MIX) <= 0)
    {
        PrintF("Test _stricmp failed\n");
        bRet = FALSE;
    }
    if (_wcsicmp(_A2W(TEST_STRING1), _A2W(TEST_STRING1_CASE_MIX)) != 0 ||
        _wcsicmp(_A2W(TEST_STRING1_SHORTER), _A2W(TEST_STRING1_CASE_MIX)) >= 0 ||
        _wcsicmp(_A2W(TEST_STRING1_LESS), _A2W(TEST_STRING1_CASE_MIX)) >= 0 ||
        _wcsicmp(_A2W(TEST_STRING1_LONGER), _A2W(TEST_STRING1_CASE_MIX)) <= 0 ||
        _wcsicmp(_A2W(TEST_STRING1_GREATER), _A2W(TEST_STRING1_CASE_MIX)) <= 0)
    {
        PrintF("Test _wcsicmp failed\n");
        bRet = FALSE;
    }
    if (_strnicmp(TEST_STRING1, TEST_STRING1_CASE_MIX, _STR_CCH_LEN(TEST_STRING1)) != 0 ||
        _strnicmp(TEST_STRING1_SHORTER, TEST_STRING1_CASE_MIX, _STR_CCH_LEN(TEST_STRING1_SHORTER)) != 0 ||
        _strnicmp(TEST_STRING1_SHORTER, TEST_STRING1_CASE_MIX, _STR_CCH_LEN(TEST_STRING1_CASE_MIX)) >= 0 ||
        _strnicmp(TEST_STRING1_CASE_MIX, TEST_STRING1_SHORTER, _STR_CCH_LEN(TEST_STRING1_CASE_MIX)) <= 0 ||
        _strnicmp(TEST_STRING1_LESS, TEST_STRING1_CASE_MIX, _STR_CCH_LEN(TEST_STRING1_CASE_MIX)) >= 0 ||
        _strnicmp(TEST_STRING1_GREATER, TEST_STRING1_CASE_MIX, _STR_CCH_LEN(TEST_STRING1_CASE_MIX)) <= 0)
    {
        PrintF("Test _strnicmp failed\n");
        bRet = FALSE;
    }
    if (_wcsnicmp(_A2W(TEST_STRING1), _A2W(TEST_STRING1_CASE_MIX), _STR_CCH_LEN(TEST_STRING1)) != 0 ||
        _wcsnicmp(_A2W(TEST_STRING1_SHORTER), _A2W(TEST_STRING1_CASE_MIX), _STR_CCH_LEN(TEST_STRING1_SHORTER)) != 0 ||
        _wcsnicmp(_A2W(TEST_STRING1_SHORTER), _A2W(TEST_STRING1_CASE_MIX), _STR_CCH_LEN(TEST_STRING1_CASE_MIX)) >= 0 ||
        _wcsnicmp(_A2W(TEST_STRING1_CASE_MIX), _A2W(TEST_STRING1_SHORTER), _STR_CCH_LEN(TEST_STRING1_CASE_MIX)) <= 0 ||
        _wcsnicmp(_A2W(TEST_STRING1_LESS), _A2W(TEST_STRING1_CASE_MIX), _STR_CCH_LEN(TEST_STRING1_CASE_MIX)) >= 0 ||
        _wcsnicmp(_A2W(TEST_STRING1_GREATER), _A2W(TEST_STRING1_CASE_MIX), _STR_CCH_LEN(TEST_STRING1_CASE_MIX)) <= 0)
    {
        PrintF("Test _wcsnicmp failed\n");
        bRet = FALSE;
    }

    /* String Copy */

    if (strcpy_s(szTempA, _countof(szTempA), TEST_STRING1) != 0 ||
        strcmp(szTempA, TEST_STRING1) != 0 ||
        strcpy_s(szTempA, _countof(szTempA), TEST_STRING1_LONGER) != ERANGE ||
        szTempA[0] != 0)
    {
        PrintF("Test strcpy_s failed\n");
        bRet = FALSE;
    }
    if (wcscpy_s(szTempW, _countof(szTempW), _A2W(TEST_STRING1)) != 0 ||
        wcscmp(szTempW, _A2W(TEST_STRING1)) != 0 ||
        wcscpy_s(szTempW, _countof(szTempW), _A2W(TEST_STRING1_LONGER)) != ERANGE ||
        szTempW[0] != 0)
    {
        PrintF("Test wcscpy_s failed\n");
        bRet = FALSE;
    }
    if (strncpy_s(szTempA, _countof(szTempA), TEST_STRING1, _STR_CCH_LEN(TEST_STRING1)) != 0 ||
        strcmp(szTempA, TEST_STRING1) != 0 ||
        strncpy_s(szTempA, _countof(szTempA), TEST_STRING1_LONGER, _STR_CCH_LEN(TEST_STRING1_LONGER)) != ERANGE ||
        szTempA[0] != 0 ||
        strncpy_s(szTempA, _countof(szTempA), TEST_STRING1_LONGER, _TRUNCATE) != STRUNCATE ||
        strcmp(szTempA, TEST_STRING1) != 0)
    {
        PrintF("Test strncpy_s failed\n");
        bRet = FALSE;
    }
    if (wcsncpy_s(szTempW, _countof(szTempW), _A2W(TEST_STRING1), _STR_CCH_LEN(TEST_STRING1)) != 0 ||
        wcscmp(szTempW, _A2W(TEST_STRING1)) != 0 ||
        wcsncpy_s(szTempW, _countof(szTempW), _A2W(TEST_STRING1_LONGER), _STR_CCH_LEN(TEST_STRING1_LONGER)) != ERANGE ||
        szTempW[0] != 0 ||
        wcsncpy_s(szTempW, _countof(szTempW), _A2W(TEST_STRING1_LONGER), _TRUNCATE) != STRUNCATE ||
        wcscmp(szTempW, _A2W(TEST_STRING1)) != 0)
    {
        PrintF("Test wcsncpy_s failed\n");
        bRet = FALSE;
    }

    /* String Concatenate */

    szTempA[0] = 0;
    if (strcat_s(szTempA, _countof(szTempA), TEST_STRING1_PART1) != 0 ||
        strcmp(szTempA, TEST_STRING1_PART1) != 0 ||
        strcat_s(szTempA, _countof(szTempA), TEST_STRING1_PART2) != 0 ||
        strcmp(szTempA, TEST_STRING1) != 0 ||
        strcat_s(szTempA, _countof(szTempA), TEST_STRING1_LONGER) != ERANGE ||
        szTempA[0] != 0)
    {
        PrintF("Test strcat_s failed\n");
        bRet = FALSE;
    }
    szTempW[0] = 0;
    if (wcscat_s(szTempW, _countof(szTempW), _A2W(TEST_STRING1_PART1)) != 0 ||
        wcscmp(szTempW, _A2W(TEST_STRING1_PART1)) != 0 ||
        wcscat_s(szTempW, _countof(szTempW), _A2W(TEST_STRING1_PART2)) != 0 ||
        wcscmp(szTempW, _A2W(TEST_STRING1)) != 0 ||
        wcscat_s(szTempW, _countof(szTempW), _A2W(TEST_STRING1_LONGER)) != ERANGE ||
        szTempW[0] != 0)
    {
        PrintF("Test wcscat_s failed\n");
        bRet = FALSE;
    }
    szTempA[0] = 0;
    if (strncat_s(szTempA, _countof(szTempA), TEST_STRING1_PART1, _STR_CCH_LEN(TEST_STRING1_PART1)) != 0 ||
        strcmp(szTempA, TEST_STRING1_PART1) != 0 ||
        strncat_s(szTempA, _countof(szTempA), TEST_STRING1_PART2, _STR_CCH_LEN(TEST_STRING1_PART2)) != 0 ||
        strcmp(szTempA, TEST_STRING1) != 0 ||
        strcpy_s(szTempA, _countof(szTempA), TEST_STRING1_PART2) != 0 ||
        strncat_s(szTempA, _countof(szTempA), TEST_STRING1_LONGER, _STR_CCH_LEN(TEST_STRING1_LONGER)) != ERANGE ||
        szTempA[0] != 0 ||
        strncat_s(szTempA, _countof(szTempA), TEST_STRING1_LONGER, _TRUNCATE) != STRUNCATE ||
        strcmp(szTempA, TEST_STRING1) != 0)
    {
        PrintF("Test strncat_s failed\n");
        bRet = FALSE;
    }
    szTempW[0] = 0;
    if (wcsncat_s(szTempW, _countof(szTempW), _A2W(TEST_STRING1_PART1), _STR_CCH_LEN(TEST_STRING1_PART1)) != 0 ||
        wcscmp(szTempW, _A2W(TEST_STRING1_PART1)) != 0 ||
        wcsncat_s(szTempW, _countof(szTempW), _A2W(TEST_STRING1_PART2), _STR_CCH_LEN(TEST_STRING1_PART2)) != 0 ||
        wcscmp(szTempW, _A2W(TEST_STRING1)) != 0 ||
        wcscpy_s(szTempW, _countof(szTempW), _A2W(TEST_STRING1_PART2)) != 0 ||
        wcsncat_s(szTempW, _countof(szTempW), _A2W(TEST_STRING1_LONGER), _STR_CCH_LEN(TEST_STRING1_LONGER)) != ERANGE ||
        szTempW[0] != 0 ||
        wcsncat_s(szTempW, _countof(szTempW), _A2W(TEST_STRING1_LONGER), _TRUNCATE) != STRUNCATE ||
        wcscmp(szTempW, _A2W(TEST_STRING1)) != 0)
    {
        PrintF("Test wcsncat_s failed\n");
        bRet = FALSE;
    }

    /*
     * String Format
     *
     * Test and make sure our own wrapper String_CchPrintf(A/W) are good and safe, instead of testing the mess in CRT
     */

    if (String_CchPrintfA(NULL, 0, "%hs", TEST_STRING1) != _STR_CCH_LEN(TEST_STRING1) ||
        String_CchPrintfA(szTempA, ARRAYSIZE(szTempA), "%hs", TEST_STRING1_PART1) != _STR_CCH_LEN(TEST_STRING1_PART1) ||
        strcmp(szTempA, TEST_STRING1_PART1) != 0 ||
        String_CchPrintfA(szTempA, ARRAYSIZE(szTempA), "%hs", TEST_STRING1) != _STR_CCH_LEN(TEST_STRING1) ||
        strcmp(szTempA, TEST_STRING1) != 0 ||
        String_CchPrintfA(szTempA, ARRAYSIZE(szTempA), "%hs$", TEST_STRING1) != _STR_CCH_LEN(TEST_STRING1) + 1 ||
        strcmp(szTempA, TEST_STRING1) != 0 ||
        String_CchPrintfA(szTempA, ARRAYSIZE(szTempA), "%hs$$", TEST_STRING1) != _STR_CCH_LEN(TEST_STRING1) + 2 ||
        strcmp(szTempA, TEST_STRING1) != 0)
    {
        PrintF("Test Str_CchPrintfA failed\n");
        bRet = FALSE;
    }

    if (String_CchPrintfW(NULL, 0, L"%ls", _A2W(TEST_STRING1)) != _STR_CCH_LEN(TEST_STRING1) ||
        String_CchPrintfW(szTempW, ARRAYSIZE(szTempW), L"%ls", _A2W(TEST_STRING1_PART1)) != _STR_CCH_LEN(TEST_STRING1_PART1) ||
        wcscmp(szTempW, _A2W(TEST_STRING1_PART1)) != 0 ||
        String_CchPrintfW(szTempW, ARRAYSIZE(szTempW), L"%ls", _A2W(TEST_STRING1)) != _STR_CCH_LEN(TEST_STRING1) ||
        wcscmp(szTempW, _A2W(TEST_STRING1)) != 0 ||
        String_CchPrintfW(szTempW, ARRAYSIZE(szTempW), L"%ls$", _A2W(TEST_STRING1)) != _STR_CCH_LEN(TEST_STRING1) + 1 ||
        wcscmp(szTempW, _A2W(TEST_STRING1)) != 0 ||
        String_CchPrintfW(szTempW, ARRAYSIZE(szTempW), L"%ls$$", _A2W(TEST_STRING1)) != _STR_CCH_LEN(TEST_STRING1) + 2 ||
        wcscmp(szTempW, _A2W(TEST_STRING1)) != 0)
    {
        PrintF("Test Str_CchPrintfW failed\n");
        bRet = FALSE;
    }

    RevertCRTErrorMode();

    return bRet;
}

