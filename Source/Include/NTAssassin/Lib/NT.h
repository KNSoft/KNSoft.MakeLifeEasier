#include "../NT/MinDef.h"

EXTERN_C_START

#pragma region Security

NTSTATUS NTAPI NT_DuplicateToken(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_TYPE TokenType,
    _Out_ PHANDLE NewTokenHandle);

NTSTATUS NTAPI NT_GetCurrentThreadToken(_Out_ PHANDLE TokenHandle);

NTSTATUS NTAPI NT_IsAdminToken(_In_ HANDLE TokenHandle);

NTSTATUS NTAPI NT_IsCurrentAdminToken();

#pragma endregion

EXTERN_C_END
