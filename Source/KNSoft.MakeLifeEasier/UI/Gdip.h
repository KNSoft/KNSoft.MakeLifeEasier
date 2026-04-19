#pragma once

#include "../MakeLifeEasier.h"

MLE_API
_Ret_maybenull_
PCWSTR
NTAPI
UI_GdipStatusInfo(
    _In_ Gdiplus::Status Status);

MLE_API
HRESULT
NTAPI
UI_GdipStatusToHr(
    _In_ Gdiplus::Status Status);

MLE_API
_Success_(return == Gdiplus::Ok)
Gdiplus::Status
NTAPI
UI_GdipStartup(
    _In_ UINT32 Version,
    _Out_ PULONG_PTR Token);

FORCEINLINE
VOID
UI_GdipShutdown(
    _In_ ULONG_PTR Token)
{
    Gdiplus::GdiplusShutdown(Token);
}

// Return FALSE to stop enumeration
typedef
_Function_class_(UI_GDIP_IMGCODECENUMPROC)
__callback
LOGICAL
CALLBACK
UI_GDIP_IMGCODECENUMPROC(
    _In_ Gdiplus::ImageCodecInfo* ImageCodecInfo,
    _In_opt_ PVOID Context);
typedef UI_GDIP_IMGCODECENUMPROC *PUI_GDIP_IMGCODECENUMPROC;

MLE_API
_Success_(return == Gdiplus::Ok)
Gdiplus::Status
NTAPI
UI_GdipEnumImageCodecs(
    _In_ LOGICAL IsEncoder,
    _In_ PUI_GDIP_IMGCODECENUMPROC ImgCodecEnumProc,
    _In_opt_ PVOID Context);

MLE_API
_Success_(return == Gdiplus::Ok)
Gdiplus::Status
NTAPI
UI_GdipGetImageCodec(
    _In_ LOGICAL IsEncoder,
    _In_ REFCLSID FormatID,
    _Out_ LPCLSID CodecClsid);

MLE_API
_Success_(return == Gdiplus::Ok)
Gdiplus::Status
NTAPI
UI_GdipSaveImageToFileEx(
    _In_ Gdiplus::Image * Image,
    _In_ PCWSTR FileName,
    _In_ REFCLSID FormatID,
    _In_opt_ Gdiplus::EncoderParameters * EncParams);
