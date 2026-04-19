#include "../MakeLifeEasier.inl"

static
PCWSTR
GdipStatusInfo[] = {
    /* Ok */                        L"The method call was successful.",
    /* GenericError */              L"There was a generic error on the method call."
    /* InvalidParameter */          L"One of the arguments passed to the method was not valid."
    /* OutOfMemory */               L"The operating system is out of memory and could not allocate memory to process the method call.",
    /* ObjectBusy */                L"One of the arguments specified in the API call is already in use in another thread.",
    /* InsufficientBuffer */        L"A buffer specified as an argument in the API call is not large enough to hold the data to be received.",
    /* NotImplemented */            L"The method is not implemented.",
    /* Win32Error */                L"The method generated a Win32 error.",
    /* WrongState */                L"The object is in an invalid state to satisfy the API call.",
    /* Aborted */                   L"The method was aborted.",
    /* FileNotFound */              L"The specified image file or metafile cannot be found.",
    /* ValueOverflow */             L"The method performed an arithmetic operation that produced a numeric overflow.",
    /* AccessDenied */              L"A write operation is not allowed on the specified file.",
    /* UnknownImageFormat */        L"The specified image file format is not known.",
    /* FontFamilyNotFound */        L"The specified font family cannot be found. Either the font family name is incorrect or the font family is not installed.",
    /* FontStyleNotFound */         L"The specified style is not available for the specified font family.",
    /* NotTrueTypeFont */           L"The font retrieved from an HDC or LOGFONT is not a TrueType font and cannot be used with GDI+.",
    /* UnsupportedGdiplusVersion */ L"The version of GDI+ that is installed on the system is incompatible with the version with which the application was compiled.",
    /* GdiplusNotInitialized */     L"The GDI+ API is not in an initialized state.",
    /* PropertyNotFound */          L"The specified property does not exist in the image.",
    /* PropertyNotSupported */      L"The specified property is not supported by the format of the image and, therefore, cannot be set.",
    /* ProfileNotFound */           L"the color profile required to save an image in CMYK format was not found."
};

_Ret_maybenull_
PCWSTR
NTAPI
UI_GdipStatusInfo(
    _In_ Gdiplus::Status Status)
{
    UINT i = static_cast<UINT>(Status);
    return i < ARRAYSIZE(GdipStatusInfo) ? GdipStatusInfo[i] : NULL;
}

HRESULT
NTAPI
UI_GdipStatusToHr(
    _In_ Gdiplus::Status Status)
{
    if (Status == Gdiplus::Ok)
    {
        return S_OK;
    } else if (Status == Gdiplus::GenericError)
    {
        return ERROR_GEN_FAILURE;
    } else if (Status == Gdiplus::InvalidParameter)
    {
        return E_INVALIDARG;
    } else if (Status == Gdiplus::OutOfMemory)
    {
        return E_OUTOFMEMORY;
    } else if (Status == Gdiplus::ObjectBusy)
    {
        return HRESULT_FROM_WIN32(ERROR_BUSY);
    } else if (Status == Gdiplus::InsufficientBuffer)
    {
        return E_NOT_SUFFICIENT_BUFFER;
    } else if (Status == Gdiplus::NotImplemented)
    {
        return E_NOTIMPL;
    } else if (Status == Gdiplus::Win32Error)
    {
        return ERROR_UNIDENTIFIED_ERROR;
    } else if (Status == Gdiplus::WrongState)
    {
        return E_NOT_VALID_STATE;
    } else if (Status == Gdiplus::Aborted)
    {
        return E_ABORT;
    } else if (Status == Gdiplus::FileNotFound)
    {
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    } else if (Status == Gdiplus::ValueOverflow)
    {
        return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
    } else if (Status == Gdiplus::AccessDenied)
    {
        return E_ACCESSDENIED;
    } else if (Status == Gdiplus::UnknownImageFormat)
    {
        return HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_TYPE);
    } else if (Status == Gdiplus::FontFamilyNotFound ||
               Status == Gdiplus::FontStyleNotFound ||
               Status == Gdiplus::PropertyNotFound ||
               Status == Gdiplus::PropertyNotSupported)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    } else if (Status == Gdiplus::NotTrueTypeFont)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATATYPE);
    } else if (Status == Gdiplus::UnsupportedGdiplusVersion)
    {
        return HRESULT_FROM_WIN32(E_INVALIDARG);
    } else if (Status == Gdiplus::GdiplusNotInitialized)
    {
        return E_ILLEGAL_METHOD_CALL;
    } else if (Status == Gdiplus::PropertyNotSupported)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    }

    return E_UNEXPECTED;
}

_Success_(return == Gdiplus::Ok)
Gdiplus::Status
NTAPI
UI_GdipStartup(
    _In_ UINT32 Version,
    _Out_ PULONG_PTR Token)
{
    if (Version != 1)
    {
        /* TODO: Use GdiplusStartupInputEx for 2 and 3 */
        return Gdiplus::UnsupportedGdiplusVersion;
    }

    Gdiplus::GdiplusStartupInput gpsi;
    gpsi.GdiplusVersion = Version;
    gpsi.DebugEventCallback = NULL;
    gpsi.SuppressBackgroundThread = FALSE;
    gpsi.SuppressExternalCodecs = FALSE;

    return Gdiplus::GdiplusStartup(Token, &gpsi, NULL);
}

_Success_(return == Gdiplus::Ok)
Gdiplus::Status
NTAPI
UI_GdipEnumImageCodecs(
    _In_ LOGICAL IsEncoder,
    _In_ PUI_GDIP_IMGCODECENUMPROC ImgCodecEnumProc,
    _In_opt_ PVOID Context)
{
    UINT uNum, uSize;
    Gdiplus::Status Status;

    Status = IsEncoder ? Gdiplus::GetImageEncodersSize(&uNum, &uSize) : Gdiplus::GetImageDecodersSize(&uNum, &uSize);
    if (Status != Gdiplus::Ok || uNum == 0 || uSize == 0)
    {
        return Status;
    }

    Gdiplus::ImageCodecInfo* pImgCodecInfo = (Gdiplus::ImageCodecInfo*)(Mem_Alloc(uSize));
    if (pImgCodecInfo == NULL)
    {
        return Gdiplus::OutOfMemory;
    }
    Status = IsEncoder ?
        Gdiplus::GetImageEncoders(uNum, uSize, pImgCodecInfo) :
        Gdiplus::GetImageDecoders(uNum, uSize, pImgCodecInfo);
    if (Status != Gdiplus::Ok)
    {
        Mem_Free(pImgCodecInfo);
        return Status;
    }
    for (UINT i = 0; i < uNum; i++)
    {
        if (!ImgCodecEnumProc(&pImgCodecInfo[i], Context))
        {
            break;
        }
    }
    Mem_Free(pImgCodecInfo);
    return Gdiplus::Ok;
}

typedef struct _UI_GDIP_IMGCODECENUMPARAM
{
    REFCLSID ClsidFormat;
    LPCLSID ClsidOut;
    BOOL Found;
} UI_GDIP_IMGCODECENUMPARAM, *PUI_GDIP_IMGCODECENUMPARAM;

static
_Function_class_(UI_GDIP_IMGCODECENUMPROC)
__callback
LOGICAL
CALLBACK
UI_GdipFindImgCodecEnumProc(
    _In_ Gdiplus::ImageCodecInfo * ImageCodecInfo,
    _In_opt_ PVOID Context)
{
    PUI_GDIP_IMGCODECENUMPARAM Param = reinterpret_cast<PUI_GDIP_IMGCODECENUMPARAM>(Context);
    if (IsEqualGUID(ImageCodecInfo->FormatID, Param->ClsidFormat))
    {
        *(Param->ClsidOut) = ImageCodecInfo->Clsid;
        Param->Found = TRUE;
        return FALSE;
    }
    return TRUE;
}

_Success_(return == Gdiplus::Ok)
Gdiplus::Status
NTAPI
UI_GdipGetImageCodec(
    _In_ LOGICAL IsEncoder,
    _In_ REFCLSID FormatID,
    _Out_ LPCLSID CodecClsid)
{
    Gdiplus::Status Status;
    UI_GDIP_IMGCODECENUMPARAM Param = { FormatID, CodecClsid, FALSE };

    Status = UI_GdipEnumImageCodecs(IsEncoder, UI_GdipFindImgCodecEnumProc, &Param);
    if (Status != Gdiplus::Ok)
    {
        return Status;
    }
    return Param.Found ? Gdiplus::Ok : Gdiplus::UnknownImageFormat;
}

_Success_(return == Gdiplus::Ok)
Gdiplus::Status
NTAPI
UI_GdipSaveImageToFileEx(
    _In_ Gdiplus::Image * Image,
    _In_ PCWSTR FileName,
    _In_ REFCLSID FormatID,
    _In_opt_ Gdiplus::EncoderParameters * EncParams)
{
    Gdiplus::Status Status;
    CLSID EncoderClsid;

    Status = UI_GdipGetImageCodec(TRUE, FormatID, &EncoderClsid);
    if (Status != Gdiplus::Ok)
    {
        return Status;
    }
    return Image->Save(FileName, &EncoderClsid, EncParams);
}
