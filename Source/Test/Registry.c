#include "Test.h"

TEST_FUNC(RegistryWin32)
{
    LSTATUS Ret;
    HKEY hKey;
    DWORD dwLsaPid, dwSize = sizeof(dwLsaPid);

    Ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Lsa", 0, KEY_QUERY_VALUE, &hKey);
    if (Ret != ERROR_SUCCESS)
    {
        TEST_SKIP("RegOpenKeyExW failed with 0x%08lX\n", Ret);
        return;
    }

    Ret = RegQueryValueExW(hKey, L"LsaPid", NULL, NULL, (LPBYTE)&dwLsaPid, &dwSize);
    if (Ret != ERROR_SUCCESS)
    {
        TEST_SKIP("RegQueryValueExW failed with 0x%08lX\n", Ret);
        return;
    }

    RegCloseKey(hKey);
}

static UNICODE_STRING g_LsaKeyPath = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\Lsa");
static UNICODE_STRING g_LsaPidKeyName = RTL_CONSTANT_STRING(L"LsaPid");

TEST_FUNC(RegistryNT)
{
    DWORD dwLsaPid;
    HANDLE hKey;
    NTSTATUS Status;

    Status = Sys_RegOpenKey(&hKey, KEY_QUERY_VALUE, &g_LsaKeyPath);
    if (!NT_SUCCESS(Status))
    {
        TEST_SKIP("Sys_RegOpenKey failed with 0x%08lX\n", Status);
        return;
    }

    Status = Sys_RegQueryDword(hKey, &g_LsaPidKeyName, &dwLsaPid);
    if (!NT_SUCCESS(Status))
    {
        TEST_SKIP("Sys_RegQueryDword failed with 0x%08lX\n", Status);
        return;
    }

    NtClose(hKey);
}
