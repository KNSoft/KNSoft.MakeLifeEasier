#include "../MakeLifeEasier.inl"

#include <roapi.h>

#pragma comment(lib, "runtimeobject.lib")

static
HRESULT
Mlep_JsonActivate(
    _In_ PCWSTR ClassName,
    _In_ ULONG Length,
    _In_ REFIID InterfaceId,
    _Outptr_ PVOID* Interface)
{
    HSTRING_HEADER Header;
    HSTRING Name;
    IInspectable* Instance;
    HRESULT Hr;

    *Interface = NULL;
    Hr = _Inline_WindowsCreateStringReference(ClassName, Length, &Header, &Name);
    if (SUCCEEDED(Hr))
    {
        Hr = RoActivateInstance(Name, &Instance);
        if (SUCCEEDED(Hr))
        {
            Hr = Instance->lpVtbl->QueryInterface(Instance, InterfaceId, Interface);
            Instance->lpVtbl->Release(Instance);
        }
    }
    return Hr;
}

static
HRESULT
Mlep_JsonUtf8ToString(
    _When_(Length == 0, _In_opt_) _When_(Length != 0, _In_reads_bytes_(Length)) PCCH Text,
    _In_ ULONG Length,
    _Out_ HSTRING* String)
{
    HSTRING_BUFFER StringBuffer;
    PWSTR Buffer;
    ULONG Bytes, Written;
    NTSTATUS Status;
    HRESULT Hr;

    *String = NULL;
    if (Length == 0)
    {
        return S_OK;
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
    Hr = _Inline_WindowsPromoteStringBuffer(StringBuffer, String);
    if (FAILED(Hr))
    {
        _Inline_WindowsDeleteStringBuffer(StringBuffer);
    }
    return Hr;
}

// Consumes the temporary reference, including when setting the member fails.
static
HRESULT
Mlep_JsonSetCreatedValue(
    _In_ IJsonObject* Object,
    _In_ PCWSTR Name,
    _In_ IJsonValue* Value)
{
    HRESULT Hr = Data_JsonObjectSetValue(Object, Name, Value);

    Value->lpVtbl->Release(Value);
    return Hr;
}

HRESULT
NTAPI
Data_JsonGetValueFactory(
    _Outptr_ IJsonValueStatics** Factory)
{
    HSTRING_HEADER Header;
    HSTRING ClassName;
    HRESULT Hr;

    *Factory = NULL;
    Hr = _Inline_WindowsCreateStringReference(RuntimeClass_Windows_Data_Json_JsonValue,
                                             _STR_LEN(RuntimeClass_Windows_Data_Json_JsonValue),
                                             &Header,
                                             &ClassName);
    if (SUCCEEDED(Hr))
    {
        Hr = RoGetActivationFactory(ClassName, &IID_IJsonValueStatics, (PVOID*)Factory);
    }
    return Hr;
}

HRESULT
NTAPI
Data_JsonCreateObject(
    _Outptr_ IJsonObject** Object)
{
    return Mlep_JsonActivate(RuntimeClass_Windows_Data_Json_JsonObject,
                             _STR_LEN(RuntimeClass_Windows_Data_Json_JsonObject),
                             &IID_IJsonObject,
                             (PVOID*)Object);
}

HRESULT
NTAPI
Data_JsonCreateArray(
    _Outptr_ IJsonVector** Array)
{
    return Mlep_JsonActivate(RuntimeClass_Windows_Data_Json_JsonArray,
                             _STR_LEN(RuntimeClass_Windows_Data_Json_JsonArray),
                             &IID_IJsonVector,
                             (PVOID*)Array);
}

HRESULT
NTAPI
Data_JsonCreateStringUtf8(
    _In_ IJsonValueStatics* Factory,
    _When_(Length == 0, _In_opt_) _When_(Length != 0, _In_reads_bytes_(Length)) PCCH Text,
    _In_ ULONG Length,
    _Outptr_ IJsonValue** Value)
{
    HSTRING String;
    HRESULT Hr;

    *Value = NULL;
    Hr = Mlep_JsonUtf8ToString(Text, Length, &String);
    if (SUCCEEDED(Hr))
    {
        Hr = Factory->lpVtbl->CreateStringValue(Factory, String, Value);
        _Inline_WindowsDeleteString(String);
    }
    return Hr;
}

HRESULT
NTAPI
Data_JsonObjectSetValue(
    _In_ IJsonObject* Object,
    _In_ PCWSTR Name,
    _In_ IJsonValue* Value)
{
    HSTRING_HEADER Header;
    HSTRING String;
    SIZE_T Length = wcslen(Name);
    HRESULT Hr;

    Hr = Length > MAXULONG ? E_INVALIDARG :
         _Inline_WindowsCreateStringReference(Name, (ULONG)Length, &Header, &String);
    return SUCCEEDED(Hr) ? Object->lpVtbl->SetNamedValue(Object, String, Value) : Hr;
}

HRESULT
NTAPI
Data_JsonObjectSetNull(
    _In_ IJsonValueStatics2* Factory,
    _In_ IJsonObject* Object,
    _In_ PCWSTR Name)
{
    IJsonValue* Value;
    HRESULT Hr = Factory->lpVtbl->CreateNullValue(Factory, &Value);

    return SUCCEEDED(Hr) ? Mlep_JsonSetCreatedValue(Object, Name, Value) : Hr;
}

HRESULT
NTAPI
Data_JsonObjectSetBoolean(
    _In_ IJsonValueStatics* Factory,
    _In_ IJsonObject* Object,
    _In_ PCWSTR Name,
    _In_ LOGICAL Value)
{
    IJsonValue* JsonValue;
    HRESULT Hr;

    Hr = Factory->lpVtbl->CreateBooleanValue(Factory, !!Value, &JsonValue);
    return SUCCEEDED(Hr) ? Mlep_JsonSetCreatedValue(Object, Name, JsonValue) : Hr;
}

HRESULT
NTAPI
Data_JsonObjectSetNumber(
    _In_ IJsonValueStatics* Factory,
    _In_ IJsonObject* Object,
    _In_ PCWSTR Name,
    _In_ DOUBLE Value)
{
    IJsonValue* JsonValue;
    HRESULT Hr;

    Hr = Factory->lpVtbl->CreateNumberValue(Factory, Value, &JsonValue);
    return SUCCEEDED(Hr) ? Mlep_JsonSetCreatedValue(Object, Name, JsonValue) : Hr;
}

HRESULT
NTAPI
Data_JsonObjectSetString(
    _In_ IJsonValueStatics* Factory,
    _In_ IJsonObject* Object,
    _In_ PCWSTR Name,
    _When_(Length == 0, _In_opt_) _When_(Length != 0, _In_reads_(Length)) PCWSTR Text,
    _In_ ULONG Length)
{
    IJsonValue* Value;
    HSTRING String;
    HRESULT Hr;

    Hr = _Inline_WindowsCreateString(Text, Length, &String);
    if (SUCCEEDED(Hr))
    {
        Hr = Factory->lpVtbl->CreateStringValue(Factory, String, &Value);
        _Inline_WindowsDeleteString(String);
    }
    return SUCCEEDED(Hr) ? Mlep_JsonSetCreatedValue(Object, Name, Value) : Hr;
}

HRESULT
NTAPI
Data_JsonObjectSetStringUtf8(
    _In_ IJsonValueStatics* Factory,
    _In_ IJsonObject* Object,
    _In_ PCWSTR Name,
    _When_(Length == 0, _In_opt_) _When_(Length != 0, _In_reads_bytes_(Length)) PCCH Text,
    _In_ ULONG Length)
{
    IJsonValue* Value;
    HRESULT Hr = Data_JsonCreateStringUtf8(Factory, Text, Length, &Value);

    return SUCCEEDED(Hr) ? Mlep_JsonSetCreatedValue(Object, Name, Value) : Hr;
}

HRESULT
NTAPI
Data_JsonStringifyUtf8(
    _In_ IJsonValue* Value,
    _Outptr_result_z_ PSTR* Text,
    _Out_opt_ PULONG Length)
{
    HSTRING String;
    PCWSTR Buffer;
    ULONG Characters, Bytes, Written;
    PSTR Utf8;
    NTSTATUS Status;
    HRESULT Hr;

    if (Length != NULL)
    {
        *Length = 0;
    }
    *Text = NULL;
    Hr = Value->lpVtbl->Stringify(Value, &String);
    if (FAILED(Hr))
    {
        return Hr;
    }
    Buffer = _Inline_WindowsGetStringRawBuffer(String, &Characters);
    if (Characters > MAXULONG / sizeof(WCHAR))
    {
        Hr = E_INVALIDARG;
        goto _Exit;
    }
    Status = RtlUnicodeToUTF8N(NULL, 0, &Bytes, Buffer, Characters * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
    {
        Hr = HRESULT_FROM_NT(Status);
        goto _Exit;
    }
    if (Bytes == MAXULONG)
    {
        Hr = HRESULT_FROM_NT(STATUS_INTEGER_OVERFLOW);
        goto _Exit;
    }
    Utf8 = Mem_Alloc((SIZE_T)Bytes + 1);
    if (Utf8 == NULL)
    {
        Hr = E_OUTOFMEMORY;
        goto _Exit;
    }
    Status = RtlUnicodeToUTF8N(Utf8, Bytes, &Written, Buffer, Characters * sizeof(WCHAR));
    if (!NT_SUCCESS(Status) || Written != Bytes)
    {
        Mem_Free(Utf8);
        Hr = !NT_SUCCESS(Status) ? HRESULT_FROM_NT(Status) : HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        goto _Exit;
    }
    Utf8[Bytes] = ANSI_NULL;
    *Text = Utf8;
    if (Length != NULL)
    {
        *Length = Bytes;
    }
    Hr = S_OK;

_Exit:
    _Inline_WindowsDeleteString(String);
    return Hr;
}

HRESULT
NTAPI
Data_JsonParse(
    _In_opt_ HSTRING Text,
    _Outptr_ IJsonValue** Value)
{
    IJsonValueStatics* Factory;
    HRESULT Hr;

    *Value = NULL;
    Hr = Data_JsonGetValueFactory(&Factory);
    if (SUCCEEDED(Hr))
    {
        Hr = Factory->lpVtbl->Parse(Factory, Text, Value);
        Factory->lpVtbl->Release(Factory);
    }
    return Hr;
}

HRESULT
NTAPI
Data_JsonParseUtf8(
    _When_(Length == 0, _In_opt_) _When_(Length != 0, _In_reads_bytes_(Length)) PCCH Text,
    _In_opt_ ULONG Length,
    _Outptr_ IJsonValue** Value)
{
    HSTRING String;
    HRESULT Hr;

    *Value = NULL;
    Hr = Mlep_JsonUtf8ToString(Text, Length, &String);
    if (SUCCEEDED(Hr))
    {
        Hr = Data_JsonParse(String, Value);
        _Inline_WindowsDeleteString(String);
    }
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

    *Value = NULL;
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
