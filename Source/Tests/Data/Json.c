#include "../Test.h"

#include <roapi.h>
#include <winstring.h>

static
DWORD
WINAPI
QueryJsonOnThread(
    _In_ PVOID Context)
{
    IJsonValue* Value = Context;
    IJsonObject* Object;
    IJsonMap* Map;
    UINT32 Size;
    HRESULT Result = RoInitialize(RO_INIT_MULTITHREADED);

    if (FAILED(Result))
    {
        return Result;
    }
    Result = Value->lpVtbl->GetObject(Value, &Object);
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Object->lpVtbl->QueryInterface(Object,
                                            &IID_IJsonMap,
                                            (PVOID*)&Map);
    Object->lpVtbl->Release(Object);
    if (SUCCEEDED(Result))
    {
        Result = Map->lpVtbl->get_Size(Map, &Size);
        if (SUCCEEDED(Result) && Size != 3)
        {
            Result = E_FAIL;
        }
        Map->lpVtbl->Release(Map);
    }
Cleanup:
    RoUninitialize();
    return Result;
}

static
VOID
TestUtf8(
    PUNITTEST_RESULT TEST_PARAMETER_RESULT)
{
    static const struct
    {
        PCSTR Text;
        ULONG Length;
    } Cases[] = {
#define JSON_CASE(text) { text, sizeof(text) - 1 }
        JSON_CASE(" "),
        JSON_CASE("\0"),
        JSON_CASE("\0{}"),
        JSON_CASE("null"),
        JSON_CASE("true"),
        JSON_CASE("12.5"),
        JSON_CASE("{}"),
        JSON_CASE("[1,false,null]"),
        JSON_CASE("\"\\u0000\""),
        JSON_CASE("\"\xE4\xB8\xAD\xF0\x9F\x98\x80\""),
        JSON_CASE("\"\xC0\xAF\""),
        JSON_CASE("\"\x80\""),
        JSON_CASE("\"\xED\xA0\x80\""),
        JSON_CASE("\"\xF4\x90\x80\x80\""),
        JSON_CASE("\"\xE2\x82\""),
        JSON_CASE("\"\xF0\x9F\""),
        JSON_CASE("\xEF\xBB\xBF{}"),
        JSON_CASE("{"),
        JSON_CASE("{}x"),
        JSON_CASE("\"a\0b\""),
        JSON_CASE("\xFF")
#undef JSON_CASE
    };
    IJsonValueStatics* Factory;
    HSTRING_HEADER Header;
    HSTRING ClassName;
    HRESULT Result;
    ULONG Index;

    Result = _Inline_WindowsCreateStringReference(RuntimeClass_Windows_Data_Json_JsonValue,
                                                  _STR_LEN(RuntimeClass_Windows_Data_Json_JsonValue),
                                                  &Header,
                                                  &ClassName);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        return;
    }
    Result = RoGetActivationFactory(ClassName,
                                    &IID_IJsonValueStatics,
                                    (PVOID*)&Factory);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        return;
    }
    for (Index = 0; Index < ARRAYSIZE(Cases); Index++)
    {
        WCHAR Buffer[128];
        HSTRING Input, ActualText, ExpectedText;
        IJsonValue* Actual = (IJsonValue*)1;
        IJsonValue* Expected;
        HRESULT ActualResult, ExpectedResult;
        int Characters;

        // Same UTF-8 policy as winrt::to_hstring: CP_UTF8 with flags == 0.
        Characters = MultiByteToWideChar(CP_UTF8,
                                         0,
                                         Cases[Index].Text,
                                         Cases[Index].Length,
                                         Buffer,
                                         ARRAYSIZE(Buffer));
        TEST_OK(Characters > 0);
        if (Characters <= 0)
        {
            continue;
        }
        Result = WindowsCreateString(Buffer, Characters, &Input);
        TEST_OK(SUCCEEDED(Result));
        if (FAILED(Result))
        {
            continue;
        }
        ExpectedResult = Factory->lpVtbl->Parse(Factory, Input, &Expected);
        ActualResult = Data_JsonParseUtf8(Cases[Index].Text, Cases[Index].Length, &Actual);
        TEST_OK(ActualResult == ExpectedResult);
        if (SUCCEEDED(ActualResult) && SUCCEEDED(ExpectedResult))
        {
            HRESULT ActualStringResult = Actual->lpVtbl->Stringify(Actual, &ActualText);
            HRESULT ExpectedStringResult = Expected->lpVtbl->Stringify(Expected, &ExpectedText);

            TEST_OK(SUCCEEDED(ActualStringResult) && SUCCEEDED(ExpectedStringResult));
            if (SUCCEEDED(ActualStringResult) && SUCCEEDED(ExpectedStringResult))
            {
                TEST_OK(_Inline_WindowsGetStringLen(ActualText) == _Inline_WindowsGetStringLen(ExpectedText));
                TEST_OK(RtlEqualMemory(_Inline_WindowsGetStringRawBuffer(ActualText, NULL),
                                      _Inline_WindowsGetStringRawBuffer(ExpectedText, NULL),
                                      _Inline_WindowsGetStringLen(ActualText) * sizeof(WCHAR)));
            }
            if (SUCCEEDED(ActualStringResult))
            {
                _Inline_WindowsDeleteString(ActualText);
            }
            if (SUCCEEDED(ExpectedStringResult))
            {
                WindowsDeleteString(ExpectedText);
            }
        }
        if (SUCCEEDED(ActualResult))
        {
            Actual->lpVtbl->Release(Actual);
        } else
        {
            TEST_OK(Actual == NULL);
        }
        if (SUCCEEDED(ExpectedResult))
        {
            Expected->lpVtbl->Release(Expected);
        }
        WindowsDeleteString(Input);
    }
    Factory->lpVtbl->Release(Factory);
}

static
VOID
TestJsonDefaults(
    PUNITTEST_RESULT TEST_PARAMETER_RESULT)
{
    static const CHAR Text[] = "{\"wrong\":42}";
    IJsonValue* Root;
    IJsonObject* Object;
    IJsonObjectWithDefaultValues* Defaults;
    IJsonArray* Array;
    HSTRING_HEADER Header;
    HSTRING Name, String;
    HRESULT Result;

    Result = Data_JsonParseUtf8(Text, sizeof(Text) - 1, &Root);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        return;
    }
    Result = Root->lpVtbl->GetObject(Root, &Object);
    Root->lpVtbl->Release(Root);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        return;
    }
    Result = Object->lpVtbl->QueryInterface(Object,
                                            &IID_IJsonObjectWithDefaultValues,
                                            (PVOID*)&Defaults);
    Object->lpVtbl->Release(Object);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        return;
    }
    // npm/dotnet optional properties use the system's default-value interface.
    Result = _Inline_WindowsCreateStringReference(L"missing", _STR_LEN(L"missing"), &Header, &Name);
    TEST_OK(SUCCEEDED(Result));
    if (SUCCEEDED(Result))
    {
        Result = Defaults->lpVtbl->GetNamedObjectOrDefault(Defaults, Name, NULL, &Object);
        TEST_OK(SUCCEEDED(Result) && Object == NULL);
        if (SUCCEEDED(Result) && Object != NULL)
        {
            Object->lpVtbl->Release(Object);
        }
        Result = Defaults->lpVtbl->GetNamedArrayOrDefault(Defaults, Name, NULL, &Array);
        TEST_OK(SUCCEEDED(Result) && Array == NULL);
        if (SUCCEEDED(Result) && Array != NULL)
        {
            Array->lpVtbl->Release(Array);
        }
        Result = Defaults->lpVtbl->GetNamedStringOrDefault(Defaults, Name, NULL, &String);
        TEST_OK(SUCCEEDED(Result) && _Inline_WindowsIsStringEmpty(String));
        if (SUCCEEDED(Result))
        {
            _Inline_WindowsDeleteString(String);
        }
    }
    Result = _Inline_WindowsCreateStringReference(L"wrong", _STR_LEN(L"wrong"), &Header, &Name);
    TEST_OK(SUCCEEDED(Result));
    if (SUCCEEDED(Result))
    {
        Result = Defaults->lpVtbl->GetNamedStringOrDefault(Defaults, Name, NULL, &String);
        TEST_OK(FAILED(Result));
        if (SUCCEEDED(Result))
        {
            _Inline_WindowsDeleteString(String);
        }
    }
    Defaults->lpVtbl->Release(Defaults);
}

static
VOID
TestJsonInputBoundary(
    PUNITTEST_RESULT TEST_PARAMETER_RESULT)
{
    PBYTE Pages = VirtualAlloc(NULL, PAGE_SIZE * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    IJsonValue* Value;
    DWORD OldProtect;
    HRESULT Result;

    TEST_OK(Pages != NULL);
    if (Pages == NULL)
    {
        return;
    }
    Pages[PAGE_SIZE - 2] = '{';
    Pages[PAGE_SIZE - 1] = '}';
    if (VirtualProtect(Pages + PAGE_SIZE, PAGE_SIZE, PAGE_NOACCESS, &OldProtect))
    {
        Result = Data_JsonParseUtf8((PCCH)Pages + PAGE_SIZE - 2, 2, &Value);
        TEST_OK(SUCCEEDED(Result));
        if (SUCCEEDED(Result))
        {
            Value->lpVtbl->Release(Value);
        }
    } else
    {
        TEST_OK(FALSE);
    }
    TEST_OK(VirtualFree(Pages, 0, MEM_RELEASE));
}

static
VOID
TestJsonStringProperty(
    PUNITTEST_RESULT TEST_PARAMETER_RESULT,
    _In_ IJsonObject* Object,
    _In_ PCWSTR Name,
    _In_reads_(Length) PCWCH Expected,
    _In_ ULONG Length)
{
    HSTRING_HEADER Header;
    HSTRING Key, String;
    HRESULT Result;

    Result = _Inline_WindowsCreateStringReference(Name, (UINT32)wcslen(Name), &Header, &Key);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        return;
    }
    Result = Object->lpVtbl->GetNamedString(Object, Key, &String);
    TEST_OK(SUCCEEDED(Result));
    if (SUCCEEDED(Result))
    {
        TEST_OK(_Inline_WindowsGetStringLen(String) == Length &&
                RtlEqualMemory(_Inline_WindowsGetStringRawBuffer(String, NULL),
                               Expected,
                               Length * sizeof(WCHAR)));
        _Inline_WindowsDeleteString(String);
    }
}

static
VOID
TestJsonConstruction(
    PUNITTEST_RESULT TEST_PARAMETER_RESULT)
{
    static const WCHAR Unicode[] = { L'A', L'\0', L'\x4E2D', L'\xD83D', L'\xDE00' };
    static const CHAR Utf8[] = "A\0\xE4\xB8\xAD\xF0\x9F\x98\x80";
    IJsonValueStatics* Factory = NULL;
    IJsonValueStatics2* Factory2 = NULL;
    IJsonObject* Object = NULL;
    IJsonObject* Child = NULL;
    IJsonVector* Vector = NULL;
    IJsonArray* Array = NULL;
    IJsonValue* Value = NULL;
    IJsonValue* Parsed = NULL;
    HSTRING_HEADER Header;
    HSTRING Name;
    HRESULT Result;
    PSTR Text = NULL;
    ULONG Length;
    UINT32 Size;
    DOUBLE Number;
    boolean Boolean;
    JsonValueType Type;

    Result = Data_JsonGetValueFactory(&Factory);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Factory->lpVtbl->QueryInterface(Factory, &IID_IJsonValueStatics2, (PVOID*)&Factory2);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Data_JsonCreateObject(&Object);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Data_JsonCreateObject(&Child);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Data_JsonCreateArray(&Vector);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    TEST_OK(SUCCEEDED(Vector->lpVtbl->get_Size(Vector, &Size)) && Size == 0);
    TEST_OK(SUCCEEDED(Data_JsonObjectSetString(Factory, Child, L"wide", Unicode, ARRAYSIZE(Unicode))));
    TEST_OK(SUCCEEDED(Data_JsonObjectSetStringUtf8(Factory, Child, L"utf8", Utf8, sizeof(Utf8) - 1)));
    TEST_OK(SUCCEEDED(Data_JsonObjectSetString(Factory, Child, L"empty", NULL, 0)));
    TEST_OK(SUCCEEDED(Data_JsonObjectSetStringUtf8(Factory, Child, L"empty8", NULL, 0)));
    TEST_OK(SUCCEEDED(Data_JsonObjectSetBoolean(Factory, Child, L"flag", TRUE)));
    TEST_OK(SUCCEEDED(Data_JsonObjectSetNull(Factory2, Child, L"nil")));
    TEST_OK(SUCCEEDED(Data_JsonObjectSetBoolean(Factory, Object, L"replace", TRUE)));
    TEST_OK(SUCCEEDED(Data_JsonObjectSetNumber(Factory, Object, L"replace", 12.5)));

    // Containers retain their own references; the same child may have multiple parents.
    Result = Child->lpVtbl->QueryInterface(Child, &IID_IJsonValue, (PVOID*)&Value);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    TEST_OK(SUCCEEDED(Data_JsonObjectSetValue(Object, L"child", Value)));
    TEST_OK(SUCCEEDED(Vector->lpVtbl->Append(Vector, Value)));
    Value->lpVtbl->Release(Value);
    Value = NULL;
    Child->lpVtbl->Release(Child);
    Child = NULL;
    Result = Factory2->lpVtbl->CreateNullValue(Factory2, &Value);
    TEST_OK(SUCCEEDED(Result));
    if (SUCCEEDED(Result))
    {
        TEST_OK(SUCCEEDED(Value->lpVtbl->get_ValueType(Value, &Type)) && Type == JsonValueType_Null);
        TEST_OK(SUCCEEDED(Vector->lpVtbl->Append(Vector, Value)));
        Value->lpVtbl->Release(Value);
        Value = NULL;
    }
    Result = Vector->lpVtbl->QueryInterface(Vector, &IID_IJsonValue, (PVOID*)&Value);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    TEST_OK(SUCCEEDED(Data_JsonObjectSetValue(Object, L"items", Value)));
    Value->lpVtbl->Release(Value);
    Value = NULL;
    Vector->lpVtbl->Release(Vector);
    Vector = NULL;
    TEST_OK(SUCCEEDED(Data_JsonObjectSetNull(Factory2, Object, L"child")));

    Result = Object->lpVtbl->QueryInterface(Object, &IID_IJsonValue, (PVOID*)&Value);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Data_JsonStringifyUtf8(Value, &Text, &Length);
    Value->lpVtbl->Release(Value);
    Value = NULL;
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    TEST_OK(Text != NULL && Length == strlen(Text) && Text[Length] == '\0');
    Result = Data_JsonParseUtf8(Text, Length, &Parsed);
    Mem_Free(Text);
    Text = NULL;
    Object->lpVtbl->Release(Object);
    Object = NULL;
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Parsed->lpVtbl->GetObject(Parsed, &Object);
    Parsed->lpVtbl->Release(Parsed);
    Parsed = NULL;
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = _Inline_WindowsCreateStringReference(L"replace", _STR_LEN(L"replace"), &Header, &Name);
    TEST_OK(SUCCEEDED(Result));
    if (SUCCEEDED(Result))
    {
        TEST_OK(SUCCEEDED(Object->lpVtbl->GetNamedNumber(Object, Name, &Number)) && Number == 12.5);
    }
    Result = _Inline_WindowsCreateStringReference(L"items", _STR_LEN(L"items"), &Header, &Name);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Object->lpVtbl->GetNamedArray(Object, Name, &Array);
    Object->lpVtbl->Release(Object);
    Object = NULL;
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Array->lpVtbl->QueryInterface(Array, &IID_IJsonVector, (PVOID*)&Vector);
    Array->lpVtbl->Release(Array);
    Array = NULL;
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    TEST_OK(SUCCEEDED(Vector->lpVtbl->get_Size(Vector, &Size)) && Size == 2);
    Result = Vector->lpVtbl->GetAt(Vector, 0, &Value);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Value->lpVtbl->GetObject(Value, &Child);
    Value->lpVtbl->Release(Value);
    Value = NULL;
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    TestJsonStringProperty(TEST_PARAMETER_RESULT, Child, L"wide", Unicode, ARRAYSIZE(Unicode));
    TestJsonStringProperty(TEST_PARAMETER_RESULT, Child, L"utf8", Unicode, ARRAYSIZE(Unicode));
    TestJsonStringProperty(TEST_PARAMETER_RESULT, Child, L"empty", L"", 0);
    TestJsonStringProperty(TEST_PARAMETER_RESULT, Child, L"empty8", L"", 0);
    Result = _Inline_WindowsCreateStringReference(L"flag", _STR_LEN(L"flag"), &Header, &Name);
    TEST_OK(SUCCEEDED(Result));
    if (SUCCEEDED(Result))
    {
        TEST_OK(SUCCEEDED(Child->lpVtbl->GetNamedBoolean(Child, Name, &Boolean)) && Boolean);
    }
    Result = Vector->lpVtbl->GetAt(Vector, 1, &Value);
    TEST_OK(SUCCEEDED(Result));
    if (SUCCEEDED(Result))
    {
        TEST_OK(SUCCEEDED(Value->lpVtbl->get_ValueType(Value, &Type)) && Type == JsonValueType_Null);
        Value->lpVtbl->Release(Value);
        Value = NULL;
    }
    Result = Vector->lpVtbl->QueryInterface(Vector, &IID_IJsonValue, (PVOID*)&Value);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Data_JsonStringifyUtf8(Value, &Text, NULL);
    Value->lpVtbl->Release(Value);
    Value = NULL;
    TEST_OK(SUCCEEDED(Result) && Text != NULL && Text[0] == '[');
    if (Text != NULL)
    {
        Mem_Free(Text);
        Text = NULL;
    }

Cleanup:
    if (Text != NULL)
    {
        Mem_Free(Text);
    }
    if (Parsed != NULL)
    {
        Parsed->lpVtbl->Release(Parsed);
    }
    if (Value != NULL)
    {
        Value->lpVtbl->Release(Value);
    }
    if (Vector != NULL)
    {
        Vector->lpVtbl->Release(Vector);
    }
    if (Array != NULL)
    {
        Array->lpVtbl->Release(Array);
    }
    if (Child != NULL)
    {
        Child->lpVtbl->Release(Child);
    }
    if (Object != NULL)
    {
        Object->lpVtbl->Release(Object);
    }
    if (Factory2 != NULL)
    {
        Factory2->lpVtbl->Release(Factory2);
    }
    if (Factory != NULL)
    {
        Factory->lpVtbl->Release(Factory);
    }
}

static
VOID
TestJsonStringBoundary(
    PUNITTEST_RESULT TEST_PARAMETER_RESULT)
{
    IJsonValueStatics* Factory = NULL;
    IJsonObject* Object = NULL;
    IJsonValue* Value;
    HSTRING String;
    PBYTE Pages = VirtualAlloc(NULL, PAGE_SIZE * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    PWSTR Wide;
    DWORD OldProtect;
    HRESULT Result;

    TEST_OK(Pages != NULL);
    if (Pages == NULL)
    {
        return;
    }
    TEST_OK(VirtualProtect(Pages + PAGE_SIZE, PAGE_SIZE, PAGE_NOACCESS, &OldProtect));
    Result = Data_JsonGetValueFactory(&Factory);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Data_JsonCreateObject(&Object);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Wide = (PWSTR)(Pages + PAGE_SIZE - 3 * sizeof(WCHAR));
    Wide[0] = L'A';
    Wide[1] = L'\0';
    Wide[2] = L'Z';
    TEST_OK(SUCCEEDED(Data_JsonObjectSetString(Factory, Object, L"boundary", Wide, 3)));
    TestJsonStringProperty(TEST_PARAMETER_RESULT, Object, L"boundary", L"A\0Z", 3);
    memcpy(Pages + PAGE_SIZE - 3, "\xE4\xB8\xAD", 3);
    Result = Data_JsonCreateStringUtf8(Factory, (PCCH)Pages + PAGE_SIZE - 3, 3, &Value);
    TEST_OK(SUCCEEDED(Result));
    if (SUCCEEDED(Result))
    {
        Result = Value->lpVtbl->GetString(Value, &String);
        Value->lpVtbl->Release(Value);
        TEST_OK(SUCCEEDED(Result));
        if (SUCCEEDED(Result))
        {
            TEST_OK(_Inline_WindowsGetStringLen(String) == 1 &&
                    _Inline_WindowsGetStringRawBuffer(String, NULL)[0] == L'\x4E2D');
            _Inline_WindowsDeleteString(String);
        }
    }

Cleanup:
    if (Object != NULL)
    {
        Object->lpVtbl->Release(Object);
    }
    if (Factory != NULL)
    {
        Factory->lpVtbl->Release(Factory);
    }
    TEST_OK(VirtualFree(Pages, 0, MEM_RELEASE));
}

static
DWORD
WINAPI
TestJsonOnStaThread(
    _In_ PVOID Context)
{
    HRESULT Result = RoInitialize(RO_INIT_SINGLETHREADED);

    if (SUCCEEDED(Result))
    {
        TestJsonConstruction(Context);
        RoUninitialize();
    }
    return Result;
}

TEST_FUNC(Data_Json)
{
    static const CHAR JsonText[] =
        "{\"text\":\"\xE4\xB8\xAD\",\"items\":[true,null,12.5],\"empty\":{}}!";
    IJsonValue* Root = NULL;
    IJsonValue* Element = NULL;
    IJsonObject* Object = NULL;
    IJsonMap* Map = NULL;
    IJsonArray* Array = NULL;
    IJsonVector* Vector = NULL;
    IJsonPairIterable* Iterable = NULL;
    IJsonPairIterator* Iterator = NULL;
    HSTRING_HEADER Header;
    HSTRING Name, String;
    JsonValueType Type;
    UINT32 Size;
    HRESULT Result;
    CO_MTA_USAGE_COOKIE MtaUsage = NULL;
    LOGICAL ApartmentInitialized = TRUE;
    HANDLE Thread;
    DWORD ThreadResult;
    boolean HasCurrent;
    ULONG Count;
    WCHAR TempPath[MAX_PATH], FilePath[MAX_PATH];
    HANDLE File;
    DWORD Written;
    UINT TempResult;

    UNREFERENCED_PARAMETER(TEST_PARAMETER_ARGC);
    UNREFERENCED_PARAMETER(TEST_PARAMETER_ARGV);
    Result = RoInitialize(RO_INIT_MULTITHREADED);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        return;
    }
    TestUtf8(TEST_PARAMETER_RESULT);
    TestJsonDefaults(TEST_PARAMETER_RESULT);
    TestJsonInputBoundary(TEST_PARAMETER_RESULT);
    TestJsonConstruction(TEST_PARAMETER_RESULT);
    TestJsonStringBoundary(TEST_PARAMETER_RESULT);
    Thread = CreateThread(NULL, 0, TestJsonOnStaThread, TEST_PARAMETER_RESULT, 0, NULL);
    if (Thread != NULL)
    {
        TEST_OK(WaitForSingleObject(Thread, INFINITE) == WAIT_OBJECT_0);
        TEST_OK(GetExitCodeThread(Thread, &ThreadResult) && SUCCEEDED((HRESULT)ThreadResult));
        NtClose(Thread);
    } else
    {
        TEST_OK(FALSE);
    }
    TEST_OK(Data_JsonParse(NULL, &Root) == WEB_E_INVALID_JSON_STRING);
    TEST_OK(Data_JsonParseUtf8(JsonText, 0, &Root) == WEB_E_INVALID_JSON_STRING);
    TEST_OK(Data_JsonParseUtf8(NULL, 0, &Root) == WEB_E_INVALID_JSON_STRING);
    Result = Data_JsonParseUtf8(JsonText, sizeof(JsonText) - 2, &Root);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    TEST_OK(SUCCEEDED(Root->lpVtbl->get_ValueType(Root, &Type)) && Type == JsonValueType_Object);
    Result = Root->lpVtbl->GetObject(Root, &Object);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Object->lpVtbl->QueryInterface(Object,
                                            &IID_IJsonMap,
                                            (PVOID*)&Map);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    TEST_OK(SUCCEEDED(Map->lpVtbl->get_Size(Map, &Size)) && Size == 3);
    Result = _Inline_WindowsCreateStringReference(L"text", 4, &Header, &Name);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Object->lpVtbl->GetNamedString(Object, Name, &String);
    TEST_OK(SUCCEEDED(Result));
    if (SUCCEEDED(Result))
    {
        TEST_OK(_Inline_WindowsGetStringLen(String) == 1 &&
                _Inline_WindowsGetStringRawBuffer(String, NULL)[0] == L'\x4E2D');
        _Inline_WindowsDeleteString(String);
    }
    Result = _Inline_WindowsCreateStringReference(L"missing", 7, &Header, &Name);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    TEST_OK(Object->lpVtbl->GetNamedValue(Object, Name, &Element) == WEB_E_JSON_VALUE_NOT_FOUND);
    Result = _Inline_WindowsCreateStringReference(L"items", 5, &Header, &Name);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Object->lpVtbl->GetNamedArray(Object, Name, &Array);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Array->lpVtbl->QueryInterface(Array,
                                           &IID_IJsonVector,
                                           (PVOID*)&Vector);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    TEST_OK(SUCCEEDED(Vector->lpVtbl->get_Size(Vector, &Size)) && Size == 3);
    Result = Vector->lpVtbl->GetAt(Vector, 1, &Element);
    TEST_OK(SUCCEEDED(Result));
    if (SUCCEEDED(Result))
    {
        TEST_OK(Element != NULL && SUCCEEDED(Element->lpVtbl->get_ValueType(Element, &Type)) &&
                Type == JsonValueType_Null);
        Element->lpVtbl->Release(Element);
        Element = NULL;
    }
    TEST_OK(Vector->lpVtbl->GetAt(Vector, 3, &Element) == E_BOUNDS);
    Result = Object->lpVtbl->QueryInterface(
        Object,
        &IID_IJsonPairIterable,
        (PVOID*)&Iterable);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Iterable->lpVtbl->First(Iterable, &Iterator);
    TEST_OK(SUCCEEDED(Result));
    if (FAILED(Result))
    {
        goto Cleanup;
    }
    Result = Iterator->lpVtbl->get_HasCurrent(Iterator, &HasCurrent);
    Count = 0;
    while (SUCCEEDED(Result) && HasCurrent)
    {
        IJsonPair* Pair;

        Result = Iterator->lpVtbl->get_Current(Iterator, &Pair);
        TEST_OK(SUCCEEDED(Result));
        if (FAILED(Result))
        {
            break;
        }
        Result = Pair->lpVtbl->get_Key(Pair, &String);
        if (SUCCEEDED(Result))
        {
            TEST_OK(_Inline_WindowsGetStringLen(String) != 0);
            _Inline_WindowsDeleteString(String);
        }
        Result = Pair->lpVtbl->get_Value(Pair, &Element);
        TEST_OK(SUCCEEDED(Result));
        if (SUCCEEDED(Result))
        {
            Element->lpVtbl->Release(Element);
            Element = NULL;
        }
        Pair->lpVtbl->Release(Pair);
        Count++;
        Result = Iterator->lpVtbl->MoveNext(Iterator, &HasCurrent);
    }
    TEST_OK(SUCCEEDED(Result) && !HasCurrent && Count == 3);
    Result = CoIncrementMTAUsage(&MtaUsage);
    TEST_OK(SUCCEEDED(Result));
    if (SUCCEEDED(Result))
    {
        RoUninitialize();
        ApartmentInitialized = FALSE;
        Thread = CreateThread(NULL, 0, QueryJsonOnThread, Root, 0, NULL);
        TEST_OK(Thread != NULL);
        if (Thread != NULL)
        {
            WaitForSingleObject(Thread, INFINITE);
            TEST_OK(GetExitCodeThread(Thread, &ThreadResult) && ThreadResult == S_OK);
            NtClose(Thread);
        }
        Result = RoInitialize(RO_INIT_MULTITHREADED);
        TEST_OK(SUCCEEDED(Result));
        ApartmentInitialized = SUCCEEDED(Result);
        if (FAILED(Result))
        {
            goto Cleanup;
        }
    }
    TempResult = GetTempPathW(ARRAYSIZE(TempPath), TempPath);
    TEST_OK(TempResult != 0 && TempResult < ARRAYSIZE(TempPath));
    if (TempResult != 0 && TempResult < ARRAYSIZE(TempPath))
    {
        TempResult = GetTempFileNameW(TempPath, L"MLE", 0, FilePath);
        TEST_OK(TempResult != 0);
        if (TempResult != 0)
        {
            TEST_OK(Data_JsonParseUtf8File(FilePath, 0, &Element) == WEB_E_INVALID_JSON_STRING);
            File = CreateFileW(FilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
            TEST_OK(File != INVALID_HANDLE_VALUE);
            if (File != INVALID_HANDLE_VALUE)
            {
                TEST_OK(WriteFile(File, JsonText, sizeof(JsonText) - 2, &Written, NULL) &&
                        Written == sizeof(JsonText) - 2);
                NtClose(File);
                Result = Data_JsonParseUtf8File(FilePath, sizeof(JsonText) - 2, &Element);
                TEST_OK(SUCCEEDED(Result));
                if (SUCCEEDED(Result))
                {
                    TEST_OK(SUCCEEDED(Element->lpVtbl->get_ValueType(Element, &Type)) &&
                            Type == JsonValueType_Object);
                    Element->lpVtbl->Release(Element);
                    Element = NULL;
                }
                TEST_OK(Data_JsonParseUtf8File(FilePath, sizeof(JsonText) - 3, &Element) ==
                        HRESULT_FROM_NT(STATUS_FILE_TOO_LARGE));
                Result = Data_JsonParseUtf8File(FilePath, 0, &Element);
                TEST_OK(SUCCEEDED(Result));
                if (SUCCEEDED(Result))
                {
                    TEST_OK(SUCCEEDED(Element->lpVtbl->get_ValueType(Element, &Type)) &&
                            Type == JsonValueType_Object);
                    Element->lpVtbl->Release(Element);
                    Element = NULL;
                }
            }
            TEST_OK(DeleteFileW(FilePath));
        }
    }
Cleanup:
    if (Iterator != NULL)
    {
        Iterator->lpVtbl->Release(Iterator);
    }
    if (Iterable != NULL)
    {
        Iterable->lpVtbl->Release(Iterable);
    }
    if (Vector != NULL)
    {
        Vector->lpVtbl->Release(Vector);
    }
    if (Array != NULL)
    {
        Array->lpVtbl->Release(Array);
    }
    if (Map != NULL)
    {
        Map->lpVtbl->Release(Map);
    }
    if (Object != NULL)
    {
        Object->lpVtbl->Release(Object);
    }
    if (Root != NULL)
    {
        Root->lpVtbl->Release(Root);
    }
    if (MtaUsage != NULL)
    {
        CoDecrementMTAUsage(MtaUsage);
    }
    if (ApartmentInitialized)
    {
        RoUninitialize();
    }
}
