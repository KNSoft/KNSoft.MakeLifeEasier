#pragma once

#include "../MakeLifeEasier.h"
#include <windows.data.json.h>

// SDK ABI aliases; these are not wrapper objects.
typedef __x_ABI_CWindows_CData_CJson_CIJsonValue IJsonValue;
typedef __x_ABI_CWindows_CData_CJson_CIJsonObject IJsonObject;
typedef __x_ABI_CWindows_CData_CJson_CIJsonObjectWithDefaultValues IJsonObjectWithDefaultValues;
typedef __x_ABI_CWindows_CData_CJson_CIJsonArray IJsonArray;
typedef __FIVector_1_Windows__CData__CJson__CIJsonValue IJsonVector;
typedef __FIMap_2_HSTRING_Windows__CData__CJson__CIJsonValue IJsonMap;
typedef __FIIterable_1___FIKeyValuePair_2_HSTRING_Windows__CData__CJson__CIJsonValue IJsonIterable;
typedef __FIIterator_1___FIKeyValuePair_2_HSTRING_Windows__CData__CJson__CIJsonValue IJsonIterator;
typedef __FIKeyValuePair_2_HSTRING_Windows__CData__CJson__CIJsonValue IJsonPair;

#ifndef __cplusplus
typedef __x_ABI_CWindows_CData_CJson_CJsonValueType JsonValueType;
#endif

// Callers initialize WinRT and keep the owning apartment alive until all interfaces are released.
// Returned interfaces own a reference. Parse borrows Text; UTF-8 decoding uses replacement characters.

EXTERN_C_START

MLE_API
HRESULT
NTAPI
Data_JsonParse(
    _In_opt_ HSTRING Text,
    _Outptr_ IJsonValue** Value);

MLE_API
HRESULT
NTAPI
Data_JsonParseUtf8(
    _In_reads_bytes_(Length) const BYTE* Text,
    _In_ ULONG Length,
    _Outptr_ IJsonValue** Value);

MLE_API
HRESULT
NTAPI
Data_JsonParseUtf8File(
    _In_ PCWSTR Path,
    _In_ ULONG MaximumSize,
    _Outptr_ IJsonValue** Value);

EXTERN_C_END
