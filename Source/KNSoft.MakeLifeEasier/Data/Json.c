#include "../MakeLifeEasier.inl"

#include <roapi.h>

#pragma comment(lib, "runtimeobject.lib")

HRESULT
NTAPI
Data_JsonParse(
    _In_opt_ HSTRING Text,
    _Outptr_ IJsonValue** Value)
{
    IJsonValueStatics* JsonValueStatics;
    HSTRING_HEADER StringHeader;
    HSTRING ClassName;
    HRESULT Hr;

    Hr = _Inline_WindowsCreateStringReference(RuntimeClass_Windows_Data_Json_JsonValue,
                                              _STR_LEN(RuntimeClass_Windows_Data_Json_JsonValue),
                                              &StringHeader,
                                              &ClassName);
    if (FAILED(Hr))
    {
        return Hr;
    }
    Hr = RoGetActivationFactory(ClassName,
                                &IID_IJsonValueStatics,
                                (PVOID*)&JsonValueStatics);
    if (FAILED(Hr))
    {
        return Hr;
    }
    Hr = JsonValueStatics->lpVtbl->Parse(JsonValueStatics, Text, Value);
    JsonValueStatics->lpVtbl->Release(JsonValueStatics);
    return Hr;
}

HRESULT
NTAPI
Data_JsonParseUtf8(
    _When_(Length == 0, _In_opt_) _When_(Length != 0, _In_reads_bytes_(Length)) PCCH Text,
    _In_opt_ ULONG Length,
    _Outptr_ IJsonValue** Value)
{
    HSTRING_BUFFER StringBuffer;
    HSTRING String;
    PWSTR Buffer;
    ULONG Bytes, Written;
    NTSTATUS Status;
    HRESULT Hr;

    if (Length == 0)
    {
        return Data_JsonParse(NULL, Value);
    }
    Status = RtlUTF8ToUnicodeN(NULL, 0, &Bytes, Text, Length);
    if (!NT_SUCCESS(Status))
    {
        return HRESULT_FROM_NT(Status);
    }
    Hr = _Inline_WindowsPreallocateStringBuffer(Bytes / sizeof(WCHAR), &Buffer, &StringBuffer);
    if (FAILED(Hr))
    {
        return Hr;
    }
    Status = RtlUTF8ToUnicodeN(Buffer, Bytes, &Written, Text, Length);
    if (!NT_SUCCESS(Status) || Written != Bytes)
    {
        _Inline_WindowsDeleteStringBuffer(StringBuffer);
        return !NT_SUCCESS(Status) ? HRESULT_FROM_NT(Status) : HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }
    Hr = _Inline_WindowsPromoteStringBuffer(StringBuffer, &String);
    if (FAILED(Hr))
    {
        _Inline_WindowsDeleteStringBuffer(StringBuffer);
        return Hr;
    }
    Hr = Data_JsonParse(String, Value);
    _Inline_WindowsDeleteString(String);
    return Hr;
}

HRESULT
NTAPI
Data_JsonParseUtf8File(
    _In_ PCWSTR Path,
    _In_opt_ ULONG MaximumSize,
    _Outptr_ IJsonValue** Value)
{
    PVOID Buffer;
    ULONGLONG FileSize;
    ULONG BytesRead;
    HANDLE File;
    NTSTATUS Status;
    HRESULT Hr;

    Status = IO_OpenWin32File(&File,
                              Path,
                              NULL,
                              FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
    if (!NT_SUCCESS(Status))
    {
        return HRESULT_FROM_NT(Status);
    }
    Status = IO_GetFileSize(File, &FileSize);
    if (!NT_SUCCESS(Status))
    {
        Hr = HRESULT_FROM_NT(Status);
        goto _Exit_0;
    }
    if (FileSize == 0)
    {
        Hr = Data_JsonParse(NULL, Value);
        goto _Exit_0;
    } else if (FileSize > MAXULONG || (MaximumSize != 0 && FileSize > MaximumSize))
    {
        Hr = HRESULT_FROM_NT(STATUS_FILE_TOO_LARGE);
        goto _Exit_0;
    }
    Buffer = Mem_Alloc((SIZE_T)FileSize);
    if (Buffer == NULL)
    {
        Hr = E_OUTOFMEMORY;
        goto _Exit_0;
    }
    Status = IO_ReadFile(File, NULL, Buffer, (ULONG)FileSize, &BytesRead);
    NtClose(File);
    if (!NT_SUCCESS(Status))
    {
        Hr = HRESULT_FROM_NT(Status);
    } else
    {
        Hr = Data_JsonParseUtf8(Buffer, BytesRead, Value);
    }
    Mem_Free(Buffer);
    return Hr;

_Exit_0:
    NtClose(File);
    return Hr;
}
