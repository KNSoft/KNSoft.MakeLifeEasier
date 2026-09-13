#pragma once

#include "../MakeLifeEasier.h"

#include <windows.data.json.h>

// SDK ABI aliases; these are not wrapper objects.
typedef __x_ABI_CWindows_CData_CJson_CIJsonValue IJsonValue;
typedef __x_ABI_CWindows_CData_CJson_CIJsonValueStatics IJsonValueStatics;
typedef __x_ABI_CWindows_CData_CJson_CIJsonValueStatics2 IJsonValueStatics2;
typedef __x_ABI_CWindows_CData_CJson_CIJsonObject IJsonObject;
typedef __x_ABI_CWindows_CData_CJson_CIJsonObjectStatics IJsonObjectStatics;
typedef __x_ABI_CWindows_CData_CJson_CIJsonObjectWithDefaultValues IJsonObjectWithDefaultValues;
typedef __x_ABI_CWindows_CData_CJson_CIJsonArray IJsonArray;
typedef __x_ABI_CWindows_CData_CJson_CIJsonArrayStatics IJsonArrayStatics;
typedef __x_ABI_CWindows_CData_CJson_CIJsonErrorStatics2 IJsonErrorStatics2;

typedef __FIVector_1_Windows__CData__CJson__CIJsonValue IJsonVector;
typedef __FIVectorView_1_Windows__CData__CJson__CIJsonValue IJsonVectorView;
typedef __FIIterable_1_Windows__CData__CJson__CIJsonValue IJsonValueIterable;
typedef __FIIterator_1_Windows__CData__CJson__CIJsonValue IJsonValueIterator;
typedef __FIMap_2_HSTRING_Windows__CData__CJson__CIJsonValue IJsonMap;
typedef __FIMapView_2_HSTRING_Windows__CData__CJson__CIJsonValue IJsonMapView;
typedef __FIKeyValuePair_2_HSTRING_Windows__CData__CJson__CIJsonValue IJsonPair;
typedef __FIIterable_1___FIKeyValuePair_2_HSTRING_Windows__CData__CJson__CIJsonValue IJsonPairIterable;
typedef __FIIterator_1___FIKeyValuePair_2_HSTRING_Windows__CData__CJson__CIJsonValue IJsonPairIterator;

#define IID_IJsonValue _Inline_IID___x_ABI_CWindows_CData_CJson_CIJsonValue
#define IID_IJsonValueStatics _Inline_IID___x_ABI_CWindows_CData_CJson_CIJsonValueStatics
#define IID_IJsonValueStatics2 _Inline_IID___x_ABI_CWindows_CData_CJson_CIJsonValueStatics2
#define IID_IJsonObject _Inline_IID___x_ABI_CWindows_CData_CJson_CIJsonObject
#define IID_IJsonObjectStatics _Inline_IID___x_ABI_CWindows_CData_CJson_CIJsonObjectStatics
#define IID_IJsonObjectWithDefaultValues _Inline_IID___x_ABI_CWindows_CData_CJson_CIJsonObjectWithDefaultValues
#define IID_IJsonArray _Inline_IID___x_ABI_CWindows_CData_CJson_CIJsonArray
#define IID_IJsonArrayStatics _Inline_IID___x_ABI_CWindows_CData_CJson_CIJsonArrayStatics
#define IID_IJsonErrorStatics2 _Inline_IID___x_ABI_CWindows_CData_CJson_CIJsonErrorStatics2
#define IID_IJsonVector _Inline_IID___FIVector_1_Windows__CData__CJson__CIJsonValue
#define IID_IJsonVectorView _Inline_IID___FIVectorView_1_Windows__CData__CJson__CIJsonValue
#define IID_IJsonValueIterable _Inline_IID___FIIterable_1_Windows__CData__CJson__CIJsonValue
#define IID_IJsonValueIterator _Inline_IID___FIIterator_1_Windows__CData__CJson__CIJsonValue
#define IID_IJsonMap _Inline_IID___FIMap_2_HSTRING_Windows__CData__CJson__CIJsonValue
#define IID_IJsonMapView _Inline_IID___FIMapView_2_HSTRING_Windows__CData__CJson__CIJsonValue
#define IID_IJsonPair _Inline_IID___FIKeyValuePair_2_HSTRING_Windows__CData__CJson__CIJsonValue
#define IID_IJsonPairIterable _Inline_IID___FIIterable_1___FIKeyValuePair_2_HSTRING_Windows__CData__CJson__CIJsonValue
#define IID_IJsonPairIterator _Inline_IID___FIIterator_1___FIKeyValuePair_2_HSTRING_Windows__CData__CJson__CIJsonValue

#ifdef __cplusplus
typedef ABI::Windows::Data::Json::JsonValueType JsonValueType;
typedef ABI::Windows::Data::Json::JsonErrorStatus JsonErrorStatus;
#else
typedef __x_ABI_CWindows_CData_CJson_CJsonValueType JsonValueType;
typedef __x_ABI_CWindows_CData_CJson_CJsonErrorStatus JsonErrorStatus;
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
    _When_(Length == 0, _In_opt_) _When_(Length != 0, _In_reads_bytes_(Length)) PCCH Text,
    _In_opt_ ULONG Length,
    _Outptr_ IJsonValue** Value);

// MaximumSize limits input bytes; zero imposes no caller-specified limit.
MLE_API
HRESULT
NTAPI
Data_JsonParseUtf8File(
    _In_ PCWSTR Path,
    _In_opt_ ULONG MaximumSize,
    _Outptr_ IJsonValue** Value);

EXTERN_C_END
