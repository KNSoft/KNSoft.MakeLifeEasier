#include "../MakeLifeEasier.inl"

NTSTATUS
NTAPI
PS_GetProcAddress(
    _In_ PVOID DllBase,
    _In_ PCANSI_STRING Name,
    _Out_ PVOID* Address)
{
    PANSI_STRING ProcName;
    ULONG ProcOridinal;

    if ((UINT_PTR)Name > MAXWORD)
    {
        ProcName = (PANSI_STRING)Name;
        ProcOridinal = 0;
    } else
    {
        ProcName = NULL;
        ProcOridinal = (ULONG)(ULONG_PTR)Name;
    }
    return LdrGetProcedureAddress(DllBase, ProcName, ProcOridinal, Address);
}
