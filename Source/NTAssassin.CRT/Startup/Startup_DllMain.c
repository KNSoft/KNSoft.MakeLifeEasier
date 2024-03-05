#include "Startup.inl"

int __stdcall _DllMainCRTStartup(
    _In_     void*         DllHandle,
    _In_     unsigned long Reason,
    _In_opt_ void*         Reserved)
{
    BOOL Return;

    if (Reason == DLL_PROCESS_ATTACH)
    {
        if (!NT_SUCCESS(CRT_Startup_Init()))
        {
            return FALSE;
        }
    }

    Return = DllMain((HMODULE)DllHandle, Reason, Reserved);

    if ((Reason == DLL_PROCESS_ATTACH && !Return) ||
        Reason == DLL_PROCESS_DETACH)
    {
        CRT_Startup_Uninit();
    }

    return Return;
}
