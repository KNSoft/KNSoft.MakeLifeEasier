#include "../MakeLifeEasier.inl"

HRESULT
NTAPI
Sys_FveEnumerateVolumes(
    _In_ __callback PSYS_FVE_VOLUME_CALLBACK Callback,
    _In_opt_ PVOID Context)
{
    FVE_FIND_DATA_V1 FindData = { FVE_FIND_VERSION_1, FVE_DEVICE_UNKNOWN };
    HANDLE FindHandle;
    ULONG NameCch;
    PWSTR Name;
    HRESULT CloseHr, Hr;

    Hr = FveFindFirstVolume(&FindHandle, &FindData);
    if (FAILED(Hr))
    {
        return Hr;
    }
    for (;;)
    {
        NameCch = 0;
        Hr = FveGetVolumeNameW(FindHandle, &NameCch, NULL);
        if (FAILED(Hr) && Hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
        {
            break;
        }
        if (NameCch == 0 || (SIZE_T)NameCch > MAXSIZE_T / sizeof(WCHAR))
        {
            Hr = E_UNEXPECTED;
            break;
        }
        Name = Mem_Alloc((SIZE_T)NameCch * sizeof(WCHAR));
        if (Name == NULL)
        {
            Hr = E_OUTOFMEMORY;
            break;
        }
        Hr = FveGetVolumeNameW(FindHandle, &NameCch, Name);
        if (SUCCEEDED(Hr))
        {
            Hr = Callback(Name, FindData.DevType, Context);
        }
        Mem_Free(Name);
        if (Hr != S_OK)
        {
            break;
        }
        Hr = FveFindNextVolume(FindHandle, &FindData);
        if (Hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES))
        {
            Hr = S_OK;
            break;
        }
        if (FAILED(Hr))
        {
            break;
        }
    }
    CloseHr = FveCloseHandle(FindHandle);
    if (SUCCEEDED(Hr) && FAILED(CloseHr))
    {
        Hr = CloseHr;
    }
    return Hr;
}

HRESULT
NTAPI
Sys_FveGetStatus(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PFVE_STATUS_V9 Status)
{
    RtlZeroMemory(Status, sizeof(*Status));
    Status->StructureSize = sizeof(*Status);
    Status->StructureVersion = FVE_STATUS_VERSION_9;
    return FveGetStatus(FveVolumeHandle, Status);
}

HRESULT
NTAPI
Sys_FveGetAuthMethodGuids(
    _In_ HANDLE FveVolumeHandle,
    _Outptr_result_buffer_maybenull_(*AuthMethodCount) PGUID* AuthMethodGuids,
    _Out_ PUINT AuthMethodCount)
{
    PGUID Guids;
    UINT Capacity, Count;
    HRESULT Hr;

    Hr = FveGetAuthMethodGuids(FveVolumeHandle, NULL, 0, &Count);
    if (Hr != S_OK && Hr != S_FALSE)
    {
        return SUCCEEDED(Hr) ? E_UNEXPECTED : Hr;
    }
    if (Count == 0)
    {
        *AuthMethodGuids = NULL;
        *AuthMethodCount = 0;
        return S_OK;
    }
    Capacity = Count;
    if ((SIZE_T)Capacity > MAXSIZE_T / sizeof(GUID))
    {
        return E_OUTOFMEMORY;
    }
    Guids = Mem_Alloc((SIZE_T)Capacity * sizeof(GUID));
    if (Guids == NULL)
    {
        return E_OUTOFMEMORY;
    }
    Hr = FveGetAuthMethodGuids(FveVolumeHandle, Guids, Capacity, &Count);
    if (Hr != S_OK)
    {
        Mem_Free(Guids);
        if (Hr == S_FALSE)
        {
            return HRESULT_FROM_WIN32(ERROR_MORE_DATA);
        }
        return SUCCEEDED(Hr) ? E_UNEXPECTED : Hr;
    }
    *AuthMethodGuids = Guids;
    *AuthMethodCount = Count;
    return S_OK;
}

HRESULT
NTAPI
Sys_FveGetAuthMethodInformation(
    _In_ HANDLE FveVolumeHandle,
    _In_opt_ PCGUID AuthMethodGuid,
    _In_ ULONG QueryFlags,
    _Outptr_result_bytebuffer_(*BufferSize) PFVE_AUTH_INFORMATION* Information,
    _Out_ PSIZE_T BufferSize)
{
    FVE_AUTH_INFORMATION Template = { 0 };
    PFVE_AUTH_INFORMATION Value;
    SIZE_T AllocationSize, RequiredSize;
    HRESULT Hr;

    Template.StructureSize = sizeof(Template);
    Template.StructureVersion = FVE_AUTH_INFORMATION_VERSION_1;
    Template.AuthFlags = QueryFlags;
    if (AuthMethodGuid != NULL)
    {
        Template.Identifier = *AuthMethodGuid;
    }
    RequiredSize = sizeof(Template);
    Hr = FveGetAuthMethodInformation(FveVolumeHandle,
                                     &Template,
                                     sizeof(Template),
                                     &RequiredSize);
    if (Hr != S_OK &&
        Hr != S_FALSE &&
        Hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
    {
        return SUCCEEDED(Hr) ? E_UNEXPECTED : Hr;
    }
    if (RequiredSize < sizeof(Template))
    {
        return E_UNEXPECTED;
    }
    AllocationSize = RequiredSize;
    Value = Mem_Alloc(AllocationSize);
    if (Value == NULL)
    {
        return E_OUTOFMEMORY;
    }
    RtlZeroMemory(Value, AllocationSize);
    RtlCopyMemory(Value, &Template, sizeof(Template));
    Hr = FveGetAuthMethodInformation(FveVolumeHandle,
                                     Value,
                                     AllocationSize,
                                     &RequiredSize);
    if (Hr != S_OK || RequiredSize > AllocationSize)
    {
        Sys_FveFreeAuthMethodInformation(Value, AllocationSize);
        if (Hr == S_OK || Hr == S_FALSE)
        {
            return HRESULT_FROM_WIN32(ERROR_MORE_DATA);
        }
        return SUCCEEDED(Hr) ? E_UNEXPECTED : Hr;
    }
    *Information = Value;
    *BufferSize = AllocationSize;
    return S_OK;
}

VOID
NTAPI
Sys_FveFreeAuthMethodInformation(
    _Frees_ptr_opt_ PFVE_AUTH_INFORMATION Information,
    _In_ SIZE_T BufferSize)
{
    if (Information != NULL)
    {
        RtlSecureZeroMemory(Information, BufferSize);
        Mem_Free(Information);
    }
}

static
HRESULT
NTAPI
Sys_FveEnsureVolumeInitialized(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCFVE_STATUS_V9 Status,
    _Out_ PLOGICAL Initialized)
{
    ULONG Flags;
    HRESULT Hr;

    if ((Status->Flags & FVE_STATUS_FLAG_INITIALIZED) != 0)
    {
        *Initialized = FALSE;
        return S_OK;
    }
    Flags = (Status->Flags & FVE_STATUS_FLAG_INITIALIZATION_UNKNOWN100) != 0 ?
                FVE_INITIALIZATION_UNKNOWN100 : 0;
    Hr = FveInitVolumeEx(FveVolumeHandle, NULL, Flags);
    if (SUCCEEDED(Hr))
    {
        *Initialized = TRUE;
    }
    return Hr;
}

static
VOID
NTAPI
Sys_FveRollbackChanges(
    _In_ HANDLE FveVolumeHandle,
    _In_ LOGICAL Initialized)
{
    if (Initialized)
    {
        FveSetFipsAllowDisabled(TRUE);
        FveRevertVolume(FveVolumeHandle);
    }
    else
    {
        FveDiscardChanges(FveVolumeHandle);
    }
}

HRESULT
NTAPI
Sys_FveAddRecoveryPasswordProtector(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCWSTR RecoveryPassword,
    _In_opt_ PCWSTR Description,
    _Out_ PGUID AuthMethodGuid)
{
    FVE_AUTH_INFORMATION Information = { 0 };
    FVE_AUTH_ELEMENT Element = { 0 };
    PFVE_AUTH_ELEMENT ElementPointer = &Element;
    FVE_STATUS_V9 Status;
    GUID Identifier;
    LOGICAL AttemptedChange = FALSE, Initialized;
    HRESULT Hr;

    Hr = Sys_FveGetStatus(FveVolumeHandle, &Status);
    if (FAILED(Hr))
    {
        return Hr;
    }
    Hr = Sys_FveEnsureVolumeInitialized(FveVolumeHandle, &Status, &Initialized);
    if (FAILED(Hr))
    {
        return Hr;
    }
    Element.StructureSize = FIELD_OFFSET(FVE_AUTH_ELEMENT, Data.RecoveryPassword) +
                            sizeof(Element.Data.RecoveryPassword);
    Element.StructureVersion = FVE_AUTH_ELEMENT_VERSION_1;
    Element.ElementType = FveAuthElementTypeRecoveryPassword;
    Hr = FveAuthElementFromRecoveryPasswordW(RecoveryPassword, &Element);
    if (SUCCEEDED(Hr))
    {
        Information.StructureSize = sizeof(Information);
        Information.StructureVersion = FVE_AUTH_INFORMATION_VERSION_1;
        Information.AuthFlags = FVE_AUTH_INFORMATION_FLAG_RECOVERY_PASSWORD;
        Information.ElementsCount = 1;
        Information.Elements = &ElementPointer;
        Information.Description = Description;
        AttemptedChange = TRUE;
        Hr = FveAddAuthMethodInformation(FveVolumeHandle, &Information, &Identifier);
    }
    if (SUCCEEDED(Hr))
    {
        Hr = FveCommitChanges(FveVolumeHandle);
    }
    if (FAILED(Hr) && (AttemptedChange || Initialized))
    {
        Sys_FveRollbackChanges(FveVolumeHandle, Initialized);
    }
    RtlSecureZeroMemory(&Element, sizeof(Element));
    if (SUCCEEDED(Hr))
    {
        *AuthMethodGuid = Identifier;
    }
    return Hr;
}

HRESULT
NTAPI
Sys_FveUnlockWithRecoveryPassword(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCWSTR RecoveryPassword)
{
    FVE_AUTH_INFORMATION Information = { 0 };
    FVE_AUTH_ELEMENT Element = { 0 };
    PFVE_AUTH_ELEMENT ElementPointer = &Element;
    FVE_STATUS_V9 Status;
    HRESULT Hr;

    Hr = Sys_FveGetStatus(FveVolumeHandle, &Status);
    if (SUCCEEDED(Hr) && (Status.Flags & FVE_STATUS_FLAG_INITIALIZED) == 0)
    {
        Hr = FVE_E_NOT_ACTIVATED;
    }
    if (SUCCEEDED(Hr))
    {
        Element.StructureSize = sizeof(Element);
        Element.StructureVersion = FVE_AUTH_ELEMENT_VERSION_1;
        Element.ElementFlags = FVE_AUTH_ELEMENT_FLAG_UNKNOWN1;
        Element.ElementType = FveAuthElementTypeRecoveryPassword;
        Hr = FveAuthElementFromRecoveryPasswordW(RecoveryPassword, &Element);
    }
    if (SUCCEEDED(Hr))
    {
        Information.StructureSize = sizeof(Information);
        Information.StructureVersion = FVE_AUTH_INFORMATION_VERSION_1;
        Information.AuthFlags = FVE_AUTH_INFORMATION_FLAG_RECOVERY_PASSWORD;
        Information.ElementsCount = 1;
        Information.Elements = &ElementPointer;
        Hr = FveUnlockVolume(FveVolumeHandle, &Information);
    }
    RtlSecureZeroMemory(&Element, sizeof(Element));
    return Hr;
}

HRESULT
NTAPI
Sys_FveDisableProtectors(
    _In_ HANDLE FveVolumeHandle,
    _In_ ULONG DisableCount)
{
    FVE_AUTH_INFORMATION Information = { 0 };
    FVE_AUTH_ELEMENT Element = { 0 };
    PFVE_AUTH_ELEMENT ElementPointer = &Element;
    FVE_STATUS_V9 Status;
    GUID Identifier;
    LOGICAL AttemptedChange = FALSE, Initialized;
    HRESULT Hr;

    if (DisableCount != SYS_FVE_DISABLE_COUNT_DEFAULT && DisableCount > 15)
    {
        return E_INVALIDARG;
    }
    Hr = Sys_FveGetStatus(FveVolumeHandle, &Status);
    if (FAILED(Hr))
    {
        return Hr;
    }
    if (DisableCount == SYS_FVE_DISABLE_COUNT_DEFAULT)
    {
        DisableCount = (Status.Flags & FVE_STATUS_FLAG_OS_VOLUME) != 0;
    }
    else if ((Status.Flags & FVE_STATUS_FLAG_OS_VOLUME) == 0)
    {
        return FVE_E_NOT_OS_VOLUME;
    }
    Hr = Sys_FveEnsureVolumeInitialized(FveVolumeHandle, &Status, &Initialized);
    if (FAILED(Hr))
    {
        return Hr;
    }
    Hr = FveSetFipsAllowDisabled(TRUE);
    if (SUCCEEDED(Hr))
    {
        Element.StructureSize = sizeof(Element);
        Element.StructureVersion = FVE_AUTH_ELEMENT_VERSION_1;
        Element.ElementFlags = FVE_AUTH_ELEMENT_FLAG_UNKNOWN1;
        Element.ElementType = FveAuthElementTypeClearKey;
        Element.Data.ClearKeyInfo.Count = (UCHAR)DisableCount;
        Information.StructureSize = sizeof(Information);
        Information.StructureVersion = FVE_AUTH_INFORMATION_VERSION_1;
        Information.AuthFlags = FVE_AUTH_INFORMATION_FLAG_CLEAR_KEY;
        Information.ElementsCount = 1;
        Information.Elements = &ElementPointer;
        AttemptedChange = TRUE;
        Hr = FveAddAuthMethodInformation(FveVolumeHandle, &Information, &Identifier);
    }
    if (SUCCEEDED(Hr))
    {
        Hr = FveCommitChanges(FveVolumeHandle);
    }
    if (FAILED(Hr) && (AttemptedChange || Initialized))
    {
        Sys_FveRollbackChanges(FveVolumeHandle, Initialized);
    }
    RtlSecureZeroMemory(&Element, sizeof(Element));
    return Hr;
}

HRESULT
NTAPI
Sys_FveEnableProtectors(
    _In_ HANDLE FveVolumeHandle)
{
    FVE_AUTH_INFORMATION Information = { 0 };
    FVE_STATUS_V9 Status;
    SIZE_T RequiredSize;
    HRESULT Hr;

    Hr = Sys_FveGetStatus(FveVolumeHandle, &Status);
    if (SUCCEEDED(Hr) && (Status.Flags & FVE_STATUS_FLAG_INITIALIZED) == 0)
    {
        Hr = FVE_E_NOT_ACTIVATED;
    }
    if (SUCCEEDED(Hr) && (Status.Flags & FVE_STATUS_FLAG_LOCKED) != 0)
    {
        Hr = FVE_E_LOCKED_VOLUME;
    }
    if (FAILED(Hr) || (Status.Flags & FVE_STATUS_FLAG_CLEAR_KEY) == 0)
    {
        return Hr;
    }
    Information.StructureSize = sizeof(Information);
    Information.StructureVersion = FVE_AUTH_INFORMATION_VERSION_1;
    Information.AuthFlags = FVE_AUTH_INFORMATION_FLAG_CLEAR_KEY |
                            FVE_AUTH_INFORMATION_QUERY_UNKNOWN2;
    RequiredSize = sizeof(Information);
    Hr = FveGetAuthMethodInformation(FveVolumeHandle,
                                     &Information,
                                     sizeof(Information),
                                     &RequiredSize);
    if (FAILED(Hr))
    {
        return Hr;
    }
    Hr = FveDeleteAuthMethod(FveVolumeHandle, &Information.Identifier);
    if (Hr == FVE_E_KEY_REQUIRED)
    {
        Hr = FVE_E_SECURE_KEY_REQUIRED;
    }
    if (SUCCEEDED(Hr))
    {
        Hr = FveCommitChanges(FveVolumeHandle);
        if (FAILED(Hr))
        {
            FveDiscardChanges(FveVolumeHandle);
        }
    }
    return Hr;
}

HRESULT
NTAPI
Sys_FveDeleteProtector(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCGUID AuthMethodGuid)
{
    PFVE_AUTH_INFORMATION Information;
    FVE_STATUS_V9 Status;
    GUID AutoUnlockGuid;
    PGUID AuthMethodGuids;
    SIZE_T InformationSize;
    UINT AuthMethodCount;
    BOOL AutoUnlockEnabled;
    LOGICAL RestoreKeyExport;
    HRESULT RestoreHr, Hr;

    Hr = Sys_FveGetStatus(FveVolumeHandle, &Status);
    if (SUCCEEDED(Hr) && (Status.Flags & FVE_STATUS_FLAG_INITIALIZED) == 0)
    {
        Hr = FVE_E_NOT_ACTIVATED;
    }
    if (FAILED(Hr))
    {
        return Hr;
    }
    Hr = FveIsBoundDataVolume(FveVolumeHandle, &AutoUnlockEnabled, &AutoUnlockGuid);
    if (Hr == FVE_E_NOT_DATA_VOLUME)
    {
        Hr = S_OK;
        AutoUnlockEnabled = FALSE;
    }
    if (FAILED(Hr))
    {
        return Hr;
    }
    if (AutoUnlockEnabled && RtlEqualMemory(AuthMethodGuid, &AutoUnlockGuid, sizeof(GUID)))
    {
        return FVE_E_VOLUME_BOUND_ALREADY;
    }
    Hr = FveSetAllowKeyExport(TRUE);
    RestoreKeyExport = Hr == S_OK;
    if (SUCCEEDED(Hr))
    {
        Hr = Sys_FveGetAuthMethodInformation(FveVolumeHandle,
                                             AuthMethodGuid,
                                             FVE_AUTH_INFORMATION_QUERY_UNKNOWN1 |
                                                 FVE_AUTH_INFORMATION_QUERY_UNKNOWN2,
                                             &Information,
                                             &InformationSize);
    }
    if (RestoreKeyExport)
    {
        RestoreHr = FveSetAllowKeyExport(FALSE);
        if (SUCCEEDED(Hr) && FAILED(RestoreHr))
        {
            Sys_FveFreeAuthMethodInformation(Information, InformationSize);
            Hr = RestoreHr;
        }
    }
    if (Hr != S_OK)
    {
        return Hr;
    }
    if ((Information->AuthFlags & FVE_AUTH_INFORMATION_PROTECTOR_MASK) == 0)
    {
        Hr = FVE_E_KEY_REQUIRED;
        goto _Free_Information;
    }
    Hr = Sys_FveGetAuthMethodGuids(FveVolumeHandle, &AuthMethodGuids, &AuthMethodCount);
    if (FAILED(Hr))
    {
        goto _Free_Information;
    }
    Mem_Free(AuthMethodGuids);
    if ((Status.Flags & FVE_STATUS_FLAG_FULLY_DECRYPTED) != 0 &&
        (AuthMethodCount == 1 ||
         (AuthMethodCount == 2 && (Status.Flags & FVE_STATUS_FLAG_CLEAR_KEY) != 0)))
    {
        Hr = FveSetFipsAllowDisabled(TRUE);
        if (SUCCEEDED(Hr))
        {
            Hr = FveRevertVolume(FveVolumeHandle);
        }
        goto _Free_Information;
    }
    if (AuthMethodCount == 1)
    {
        Hr = FVE_E_KEY_REQUIRED;
        goto _Free_Information;
    }
    Hr = FveDeleteAuthMethod(FveVolumeHandle, AuthMethodGuid);
    if (SUCCEEDED(Hr))
    {
        Hr = FveCommitChanges(FveVolumeHandle);
        if (FAILED(Hr))
        {
            FveDiscardChanges(FveVolumeHandle);
        }
    }

_Free_Information:
    Sys_FveFreeAuthMethodInformation(Information, InformationSize);
    return Hr;
}

HRESULT
NTAPI
Sys_FveEncrypt(
    _In_ HANDLE FveVolumeHandle,
    _In_ FVE_LEGACY_METHOD FveMethod,
    _In_ LOGICAL DataOnly)
{
    FVE_STATUS_V9 Status;
    FVE_LEGACY_METHOD CurrentMethod;
    ULONG Flags;
    HRESULT Hr;

    Hr = Sys_FveGetStatus(FveVolumeHandle, &Status);
    if (FAILED(Hr))
    {
        return Hr;
    }
    if ((Status.Flags & FVE_STATUS_FLAG_INITIALIZED) == 0)
    {
        return FVE_E_CANNOT_ENCRYPT_NO_KEY;
    }
    if ((Status.Flags & FVE_STATUS_FLAG_LOCKED) != 0)
    {
        return FVE_E_LOCKED_VOLUME;
    }
    Flags = DataOnly ? FVE_CONVERSION_FLAG_DATA_ONLY : 0;
    if ((Status.Flags & FVE_STATUS_FLAG_FULLY_DECRYPTED) != 0)
    {
        Hr = FveSetFveMethod(FveVolumeHandle, FveMethod);
        if (SUCCEEDED(Hr))
        {
            Hr = FveCommitChanges(FveVolumeHandle);
            if (FAILED(Hr))
            {
                FveDiscardChanges(FveVolumeHandle);
            }
        }
        if (SUCCEEDED(Hr))
        {
            Hr = FveConversionEncryptEx(FveVolumeHandle, Flags);
        }
        return Hr;
    }
    Hr = FveGetFveMethod(FveVolumeHandle, &CurrentMethod);
    if (FAILED(Hr))
    {
        return Hr;
    }
    if (FveMethod != FveLegacyMethodNone && CurrentMethod != FveMethod)
    {
        return E_INVALIDARG;
    }
    if ((Status.Flags & FVE_STATUS_FLAG_FULLY_ENCRYPTED) != 0 ||
        ((Status.Flags & FVE_STATUS_FLAG_ENCRYPTION_IN_PROGRESS) != 0 &&
         (Status.Flags & FVE_STATUS_FLAG_CONVERSION_PAUSED_MASK) == 0))
    {
        return S_OK;
    }
    if ((Status.Flags & FVE_STATUS_FLAG_DECRYPTION_IN_PROGRESS) != 0)
    {
        Hr = FveConversionStop(FveVolumeHandle);
        if (FAILED(Hr))
        {
            return Hr;
        }
    }
    else if ((Status.Flags & FVE_STATUS_FLAG_ENCRYPTION_IN_PROGRESS) == 0)
    {
        return E_UNEXPECTED;
    }
    return FveConversionEncryptEx(FveVolumeHandle, Flags);
}

HRESULT
NTAPI
Sys_FveDecrypt(
    _In_ HANDLE FveVolumeHandle)
{
    FVE_STATUS_V9 Status;
    HRESULT Hr;

    Hr = Sys_FveGetStatus(FveVolumeHandle, &Status);
    if (FAILED(Hr))
    {
        return Hr;
    }
    if ((Status.Flags & FVE_STATUS_FLAG_INITIALIZED) == 0)
    {
        return S_OK;
    }
    Hr = FveConversionDecrypt(FveVolumeHandle);
    if (FAILED(Hr))
    {
        return Hr;
    }
    Hr = Sys_FveGetStatus(FveVolumeHandle, &Status);
    if (FAILED(Hr) || (Status.Flags & FVE_STATUS_FLAG_FULLY_DECRYPTED) == 0)
    {
        return Hr;
    }
    Hr = FveSetFipsAllowDisabled(TRUE);
    return FAILED(Hr) ? Hr : FveRevertVolume(FveVolumeHandle);
}
