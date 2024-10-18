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
PS_OpenCurrentThreadToken(
    _Out_ PHANDLE TokenHandle);

MLE_API
NTSTATUS
NTAPI
PS_IsAdminToken(
    _In_ HANDLE TokenHandle);

MLE_API
NTSTATUS
NTAPI
PS_IsCurrentAdminToken(VOID);

EXTERN_C_END
