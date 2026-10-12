#include "../MakeLifeEasier.inl"

CONST OBJECT_ATTRIBUTES NT_EmptyObjectAttribute = RTL_INIT_OBJECT_ATTRIBUTES(NULL, 0);

NTSTATUS
NTAPI
NT_Win32PathToNtPath(
    _In_ PCWSTR Win32Path,
    _Out_ PUNICODE_STRING NtPath)
{
    WCHAR Buffer[MAX_PATH];
    UNICODE_STRING DosName;
    UNICODE_STRING FullName = { 0, sizeof(Buffer), Buffer };
    PCUNICODE_STRING Prefix;
    PWSTR NewBuffer;
    RTL_PATH_TYPE InputType = RtlPathTypeUnknown;
    BOOLEAN NameInvalid;
    SIZE_T BytesRequired, Skip, Size;
    NTSTATUS Status;

    Status = RtlDosPathNameToNtPathName_U_WithStatus(Win32Path, NtPath, NULL, NULL);
    if (Status != STATUS_NAME_TOO_LONG)
    {
        return Status;
    }
    Status = _Inline_RtlInitUnicodeStringEx(&DosName, Win32Path);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    /* Query the exact capacity; RTL's automatic allocation reserves two extra bytes. */
    Status = RtlGetFullPathName_UstrEx(&DosName,
                                       &FullName,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &NameInvalid,
                                       &InputType,
                                       &BytesRequired);
    if (NameInvalid)
    {
        return STATUS_OBJECT_NAME_INVALID;
    }
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        if (BytesRequired > UNICODE_STRING_MAX_BYTES)
        {
            return STATUS_NAME_TOO_LONG;
        }
        FullName.MaximumLength = (USHORT)BytesRequired;
        FullName.Buffer = RtlAllocateHeap(RtlProcessHeap(), 0, BytesRequired);
        if (FullName.Buffer == NULL)
        {
            return STATUS_NO_MEMORY;
        }
        do
        {
            Status = RtlGetFullPathName_UstrEx(&DosName,
                                               &FullName,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &NameInvalid,
                                               &InputType,
                                               &BytesRequired);
            if (NameInvalid)
            {
                Status = STATUS_OBJECT_NAME_INVALID;
                goto _Exit;
            }
            if (Status == STATUS_BUFFER_TOO_SMALL)
            {
                if (BytesRequired > UNICODE_STRING_MAX_BYTES)
                {
                    Status = STATUS_NAME_TOO_LONG;
                    goto _Exit;
                }
                NewBuffer = RtlAllocateHeap(RtlProcessHeap(), 0, BytesRequired);
                if (NewBuffer == NULL)
                {
                    Status = STATUS_NO_MEMORY;
                    goto _Exit;
                }
                RtlFreeHeap(RtlProcessHeap(), 0, FullName.Buffer);
                FullName.Buffer = NewBuffer;
                FullName.MaximumLength = (USHORT)BytesRequired;
            }
        } while (Status == STATUS_BUFFER_TOO_SMALL);
        if (!NT_SUCCESS(Status))
        {
            goto _Exit;
        }
    } else if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Keep the DOS parsing in RTL; only replace the normalized namespace prefix. */
    Prefix = &_Inline_RtlDosPathPrefix;
    switch (_Inline_RtlDetermineDosPathNameType_Ustr(&FullName))
    {
        case RtlPathTypeUncAbsolute:
            Prefix = &_Inline_RtlUncPathPrefix;
            Skip = 2 * sizeof(WCHAR);
            break;
        case RtlPathTypeLocalDevice:
            Skip = 4 * sizeof(WCHAR);
            break;
        default:
            Skip = 0;
            break;
    }
    Size = (SIZE_T)Prefix->Length + FullName.Length - Skip + sizeof(WCHAR);
    if (Size > UNICODE_STRING_MAX_BYTES)
    {
        Status = STATUS_NAME_TOO_LONG;
        goto _Exit;
    }
    NtPath->Buffer = RtlAllocateHeap(RtlProcessHeap(), 0, Size);
    if (NtPath->Buffer == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto _Exit;
    }
    NtPath->Length = (USHORT)(Size - sizeof(WCHAR));
    NtPath->MaximumLength = (USHORT)Size;
    RtlCopyMemory(NtPath->Buffer, Prefix->Buffer, Prefix->Length);
    RtlCopyMemory((PUCHAR)NtPath->Buffer + Prefix->Length,
                  (PUCHAR)FullName.Buffer + Skip,
                  FullName.Length - Skip);
    NtPath->Buffer[NtPath->Length / sizeof(WCHAR)] = UNICODE_NULL;
    Status = STATUS_SUCCESS;

_Exit:
    if (FullName.Buffer != Buffer)
    {
        RtlFreeHeap(RtlProcessHeap(), 0, FullName.Buffer);
    }
    return Status;
}
