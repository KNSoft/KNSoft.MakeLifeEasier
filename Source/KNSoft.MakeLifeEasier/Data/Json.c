#include "../MakeLifeEasier.inl"

#include <roapi.h>

#pragma comment(lib, "runtimeobject.lib")

// SDK windows.data.json.h declares these IIDs, but the SDK import libraries do not define them.
const IID IID___x_ABI_CWindows_CData_CJson_CIJsonValueStatics =
    { 0x5F6B544A, 0x2F53, 0x48E1, { 0x91, 0xA3, 0xF7, 0x8B, 0x50, 0xA6, 0x34, 0x5C } };
const IID IID___x_ABI_CWindows_CData_CJson_CIJsonObjectWithDefaultValues =
    { 0xD960D2A2, 0xB7F0, 0x4F00, { 0x8E, 0x44, 0xD8, 0x2C, 0xF4, 0x15, 0xEA, 0x13 } };
const IID IID___FIMap_2_HSTRING_Windows__CData__CJson__CIJsonValue =
    { 0xC9D9A725, 0x786B, 0x5113, { 0xB4, 0xB7, 0x9B, 0x61, 0x76, 0x4C, 0x22, 0x0B } };
const IID IID___FIVector_1_Windows__CData__CJson__CIJsonValue =
    { 0xD44662BC, 0xDCE3, 0x59A8, { 0x92, 0x72, 0x4B, 0x21, 0x0F, 0x33, 0x90, 0x8B } };
const IID IID___FIIterable_1___FIKeyValuePair_2_HSTRING_Windows__CData__CJson__CIJsonValue =
    { 0xDFABB6E1, 0x0411, 0x5A8F, { 0xAA, 0x87, 0x35, 0x4E, 0x71, 0x10, 0xF0, 0x99 } };

HRESULT
NTAPI
Data_JsonParse(
    _In_opt_ HSTRING Text,
    _Outptr_ IJsonValue** Value)
{
    __x_ABI_CWindows_CData_CJson_CIJsonValueStatics* Factory;
    HSTRING_HEADER Header;
    HSTRING ClassName;
    HRESULT Result;

    if (Value == NULL)
    {
        return E_INVALIDARG;
    }
    Result = _Inline_WindowsCreateStringReference(RuntimeClass_Windows_Data_Json_JsonValue,
                                                 _STR_LEN(RuntimeClass_Windows_Data_Json_JsonValue),
                                                 &Header,
                                                 &ClassName);
    if (FAILED(Result))
    {
        return Result;
    }
    Result = RoGetActivationFactory(ClassName,
                                    &IID___x_ABI_CWindows_CData_CJson_CIJsonValueStatics,
                                    (PVOID*)&Factory);
    if (FAILED(Result))
    {
        return Result;
    }
    Result = Factory->lpVtbl->Parse(Factory, Text, Value);
    Factory->lpVtbl->Release(Factory);
    return Result;
}

HRESULT
NTAPI
Data_JsonParseUtf8(
    _In_reads_bytes_(Length) const BYTE* Text,
    _In_ ULONG Length,
    _Outptr_ IJsonValue** Value)
{
    HSTRING_BUFFER Handle;
    HSTRING String;
    PWSTR Buffer;
    ULONG Bytes, Written;
    NTSTATUS Status;
    HRESULT Result;

    if (Text == NULL || Value == NULL)
    {
        return E_INVALIDARG;
    }
    if (Length == 0)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }
    Status = RtlUTF8ToUnicodeN(NULL, 0, &Bytes, (PCCH)Text, Length);
    if (!NT_SUCCESS(Status))
    {
        return HRESULT_FROM_NT(Status);
    }
    Result = _Inline_WindowsPreallocateStringBuffer(Bytes / sizeof(WCHAR), &Buffer, &Handle);
    if (FAILED(Result))
    {
        return Result;
    }
    Status = RtlUTF8ToUnicodeN(Buffer, Bytes, &Written, (PCCH)Text, Length);
    if (!NT_SUCCESS(Status) || Written != Bytes)
    {
        _Inline_WindowsDeleteStringBuffer(Handle);
        return !NT_SUCCESS(Status) ? HRESULT_FROM_NT(Status) : HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }
    Result = _Inline_WindowsPromoteStringBuffer(Handle, &String);
    if (FAILED(Result))
    {
        _Inline_WindowsDeleteStringBuffer(Handle);
        return Result;
    }
    Result = Data_JsonParse(String, Value);
    _Inline_WindowsDeleteString(String);
    return Result;
}

HRESULT
NTAPI
Data_JsonParseUtf8File(
    _In_ PCWSTR Path,
    _In_ ULONG MaximumSize,
    _Outptr_ IJsonValue** Value)
{
    PBYTE Buffer = NULL;
    ULONGLONG FileSize;
    ULONG BytesRead;
    HANDLE File;
    NTSTATUS Status;
    HRESULT Result;

    if (Path == NULL || MaximumSize == 0 || Value == NULL)
    {
        return E_INVALIDARG;
    }
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
    if (NT_SUCCESS(Status) && FileSize == 0)
    {
        Status = STATUS_DATA_ERROR;
    }
    if (NT_SUCCESS(Status) && FileSize > MaximumSize)
    {
        Status = STATUS_FILE_TOO_LARGE;
    }
    if (NT_SUCCESS(Status))
    {
        Buffer = Mem_Alloc((SIZE_T)FileSize);
        if (Buffer == NULL)
        {
            Status = STATUS_NO_MEMORY;
        }
    }
    if (NT_SUCCESS(Status))
    {
        Status = IO_ReadFile(File, NULL, Buffer, (ULONG)FileSize, &BytesRead);
    }
    NtClose(File);
    if (!NT_SUCCESS(Status))
    {
        Result = HRESULT_FROM_NT(Status);
    } else if (BytesRead == 0)
    {
        Result = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    } else
    {
        Result = Data_JsonParseUtf8(Buffer, BytesRead, Value);
    }
    if (Buffer != NULL)
    {
        Mem_Free(Buffer);
    }
    return Result;
}
