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
                                            &IID___FIMap_2_HSTRING_Windows__CData__CJson__CIJsonValue,
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
    __x_ABI_CWindows_CData_CJson_CIJsonValueStatics* Factory;
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
                                    &IID___x_ABI_CWindows_CData_CJson_CIJsonValueStatics,
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
        IJsonValue* Actual;
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
        ActualResult = Data_JsonParseUtf8((const BYTE*)Cases[Index].Text, Cases[Index].Length, &Actual);
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
    static const BYTE Text[] = "{\"wrong\":42}";
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
                                            &IID___x_ABI_CWindows_CData_CJson_CIJsonObjectWithDefaultValues,
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
        Result = Data_JsonParseUtf8(Pages + PAGE_SIZE - 2, 2, &Value);
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

TEST_FUNC(Data_Json)
{
    static const BYTE JsonText[] =
        "{\"text\":\"\xE4\xB8\xAD\",\"items\":[true,null,12.5],\"empty\":{}}!";
    IJsonValue* Root = NULL;
    IJsonValue* Element = NULL;
    IJsonObject* Object = NULL;
    IJsonMap* Map = NULL;
    IJsonArray* Array = NULL;
    IJsonVector* Vector = NULL;
    IJsonIterable* Iterable = NULL;
    IJsonIterator* Iterator = NULL;
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
    TEST_OK(Data_JsonParseUtf8(JsonText, 0, &Root) == HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
    TEST_OK(Data_JsonParseUtf8(NULL, 1, &Root) == E_INVALIDARG);
    TEST_OK(Data_JsonParseUtf8(JsonText, sizeof(JsonText) - 2, NULL) == E_INVALIDARG);
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
                                            &IID___FIMap_2_HSTRING_Windows__CData__CJson__CIJsonValue,
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
                                           &IID___FIVector_1_Windows__CData__CJson__CIJsonValue,
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
        &IID___FIIterable_1___FIKeyValuePair_2_HSTRING_Windows__CData__CJson__CIJsonValue,
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
