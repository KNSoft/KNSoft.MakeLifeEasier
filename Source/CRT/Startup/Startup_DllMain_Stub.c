#include "../CRT.inl"

BOOL
WINAPI
DllMain(
    _In_ HINSTANCE  hInstance,
    _In_ DWORD      fdwReason,
    _In_opt_ LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        LdrDisableThreadCalloutsForDll(hInstance);
    }
    return TRUE;
}
