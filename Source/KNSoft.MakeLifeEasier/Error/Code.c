#include "../MakeLifeEasier.inl"

NTSTATUS
NTAPI
Err_HrToNtStatus(
    _In_ HRESULT Hr)
{
    if (Hr == S_OK)
    {
        return STATUS_SUCCESS;
    } else if (Hr == E_INVALIDARG)
    {
        return STATUS_INVALID_PARAMETER;
    } else if (Hr == HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR))
    {
        return STATUS_INTERNAL_ERROR;
    } else if (Hr == E_OUTOFMEMORY)
    {
        return STATUS_NO_MEMORY;
    } else if (Hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW))
    {
        return STATUS_INTEGER_OVERFLOW;
    } else if (Hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
    {
        return STATUS_OBJECT_PATH_NOT_FOUND;
    } else if (Hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    } else if (Hr == HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION))
    {
        return STATUS_NOT_IMPLEMENTED;
    } else if (Hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA))
    {
        return STATUS_BUFFER_OVERFLOW;
    } else if (Hr == HRESULT_FROM_WIN32(ERROR_IMPLEMENTATION_LIMIT))
    {
        return STATUS_IMPLEMENTATION_LIMIT;
    } else if (Hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_MATCHES))
    {
        return STATUS_NO_MORE_MATCHES;
    } else if (Hr == HRESULT_FROM_WIN32(ERROR_ILLEGAL_CHARACTER))
    {
        return STATUS_ILLEGAL_CHARACTER;
    } else if (Hr == HRESULT_FROM_WIN32(ERROR_UNDEFINED_CHARACTER))
    {
        return STATUS_UNDEFINED_CHARACTER;
    } else if (Hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
    {
        return STATUS_BUFFER_TOO_SMALL;
    } else if (Hr == HRESULT_FROM_WIN32(ERROR_DISK_FULL))
    {
        return STATUS_DISK_FULL;
    } else if (Hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME))
    {
        return STATUS_OBJECT_NAME_INVALID;
    } else if (Hr == HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND))
    {
        return STATUS_DLL_NOT_FOUND;
    } else if (Hr == HRESULT_FROM_WIN32(ERROR_OLD_WIN_VERSION))
    {
        return STATUS_REVISION_MISMATCH;
    } else if (Hr == E_FAIL)
    {
        return STATUS_UNSUCCESSFUL;
    } else if (Hr == HRESULT_FROM_WIN32(ERROR_XML_PARSE_ERROR))
    {
        return STATUS_XML_PARSE_ERROR;
    } else if (Hr == HRESULT_FROM_WIN32(ERROR_UNHANDLED_EXCEPTION))
    {
        return STATUS_NONCONTINUABLE_EXCEPTION;
    }

    if ((ULONG)Hr & FACILITY_NT_BIT)
    {
        return Hr & ~FACILITY_NT_BIT;
    } else if (HRESULT_FACILITY(Hr) == FACILITY_WIN32)
    {
        return NTSTATUS_FROM_WIN32(HRESULT_CODE(Hr));
    } else if (HRESULT_FACILITY(Hr) == FACILITY_SSPI)
    {
        return Hr <= 0 ? (NTSTATUS)Hr : MAKE_NTSTATUS(STATUS_SEVERITY_ERROR, FACILITY_SSPI, HRESULT_CODE(Hr));
    }
    return STATUS_INTERNAL_ERROR;
}
