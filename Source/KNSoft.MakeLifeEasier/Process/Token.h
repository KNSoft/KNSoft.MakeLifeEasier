#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

MLE_API
NTSTATUS
NTAPI
PS_DuplicateToken(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_TYPE TokenType,
    _Out_ PHANDLE NewTokenHandle);

MLE_API
NTSTATUS
NTAPI
PS_DuplicateSystemToken(
    _In_ ULONG ProcessId,
    _In_ TOKEN_TYPE TokenType,
    _Out_ PHANDLE NewTokenHandle);

MLE_API
NTSTATUS
NTAPI
PS_OpenCurrentThreadToken(
    _Out_ PHANDLE TokenHandle);

MLE_API
NTSTATUS
NTAPI
PS_IsAdminToken(
    _In_ HANDLE TokenHandle);

FORCEINLINE
NTSTATUS
PS_IsCurrentAdminToken(VOID)
{
    NTSTATUS Status;
    HANDLE Token;

    Status = PS_OpenCurrentThreadToken(&Token);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = PS_IsAdminToken(Token);
    NtClose(Token);
    return Status;
}

MLE_API
NTSTATUS
NTAPI
PS_GetTokenInfo(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
    _Out_ PVOID* Info);

/// <summary>
/// Set impersonate token to current thread
/// </summary>
/// <param name="TokenHandle">Handle to the impersonate token, or NULL to revert the impersonation</param>
FORCEINLINE
NTSTATUS
PS_Impersonate(
    _In_opt_ HANDLE TokenHandle)
{
    return NtSetInformationThread(NtCurrentThread(), ThreadImpersonationToken, &TokenHandle, sizeof(TokenHandle));
}

FORCEINLINE
NTSTATUS
PS_AdjustPrivilege(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG Privilege,
    _In_ LOGICAL Enable)
{
    NTSTATUS Status;
    HANDLE TokenHandle;

    Status = NtOpenProcessToken(ProcessHandle, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &TokenHandle);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    Status = NT_AdjustTokenPrivilege(TokenHandle, Privilege, Enable ? SE_PRIVILEGE_ENABLED : 0);
    NtClose(TokenHandle);

    return Status;
}

EXTERN_C_END
