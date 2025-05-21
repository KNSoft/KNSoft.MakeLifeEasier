#include "../MakeLifeEasier.inl"

#include <shellapi.h>
#include <Shlobj.h>

W32ERROR
NTAPI
Shell_Exec(
    _In_ PCWSTR File,
    _In_opt_ PCWSTR Param,
    _In_opt_ PCWSTR Verb,
    _In_ INT ShowCmd,
    _Out_opt_ PHANDLE ProcessHandle)
{
    SHELLEXECUTEINFOW SEI = {
        .cbSize = sizeof(SHELLEXECUTEINFOW),
        .fMask = (ProcessHandle != NULL ? SEE_MASK_NOCLOSEPROCESS : SEE_MASK_DEFAULT) | SEE_MASK_INVOKEIDLIST,
        .lpFile = File,
        .lpParameters = Param,
        .lpVerb = Verb,
        .nShow = ShowCmd
    };

    if (ShellExecuteExW(&SEI))
    {
        if (ProcessHandle != NULL)
        {
            *ProcessHandle = SEI.hProcess;
        }
        return ERROR_SUCCESS;
    } else
    {
        return Err_GetLastError();
    }
}

HRESULT
NTAPI
Shell_LocateItem(
    _In_ PCWSTR Path)
{
    HRESULT hr;
    WCHAR DirPath[MAX_PATH], wc;
    SFGAOF Attribute;
    PIDLIST_ABSOLUTE pidlDir, pidlFile;
    SIZE_T i, j;

    i = j = 0;
    do
    {
        wc = Path[i];
        DirPath[i] = wc;
        if (wc == L'\\' || wc == L'/')
        {
            j = i;
        }
        i++;
    } while (wc != UNICODE_NULL);

    if (j > 0)
    {
        DirPath[j] = UNICODE_NULL;
        hr = SHParseDisplayName(DirPath, NULL, &pidlDir, 0, &Attribute);
        if (hr == S_OK)
        {
            hr = SHParseDisplayName(Path, NULL, &pidlFile, 0, &Attribute);
            if (hr == S_OK)
            {
                hr = SHOpenFolderAndSelectItems(pidlDir, 1, &pidlFile, 0);
                CoTaskMemFree(pidlFile);
            }
            CoTaskMemFree(pidlDir);
        }
    } else
    {
        hr = CO_E_BAD_PATH;
    }

    return hr;
}

HRESULT
NTAPI
Shell_GetLinkPath(
    _In_ PCWSTR LinkFile,
    _Out_writes_(PathCchSize) PWSTR Path,
    _In_ INT PathCchSize)
{
    HRESULT hr;
    IShellLinkW* psl;
    IPersistFile* ppf;

    hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, &psl);
    if (SUCCEEDED(hr))
    {
        hr = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, &ppf);
        if (SUCCEEDED(hr))
        {
            hr = ppf->lpVtbl->Load(ppf, LinkFile, STGM_READ);
            if (SUCCEEDED(hr))
            {
                hr = psl->lpVtbl->GetPath(psl, Path, PathCchSize, NULL, 0);
            }
            ppf->lpVtbl->Release(ppf);
        }
        psl->lpVtbl->Release(psl);
    }

    return hr;
}
