#include "../MakeLifeEasier.inl"

template <typename TChar>
static
_Success_(return != FALSE)
LOGICAL
NTAPI
Str_ToIntEx_Impl(
    _In_ CONST TChar* StrValue,
    _In_ BOOL Unsigned,
    _In_ ULONG Base,
    _Out_writes_bytes_(ValueSize) PVOID Value,
    _In_ ULONG ValueSize)
{
    CONST TChar* psz = StrValue;
    UINT64 uTotal;
    BOOL bMinus;
    UINT64 uMax;
    ULONG uBase;
    TChar ch;
    USHORT uc;
    UINT64 uTemp;

    // Minus
    if (*psz == (TChar)'\0')
    {
        return FALSE;
    }
    if (*psz == (TChar)'-')
    {
        if (!Unsigned)
        {
            psz++;
            bMinus = TRUE;
        } else
        {
            return FALSE;
        }
    } else if (*psz == (TChar)'+')
    {
        psz++;
        bMinus = FALSE;
    } else
    {
        bMinus = FALSE;
    }

    // Base
    if (Base == 0)
    {
        if (*psz == (TChar)'\0')
        {
            return FALSE;
        }
        if (*psz == (TChar)'0')
        {
            psz++;
            if (*psz == (TChar)'\0')
            {
                uTotal = 0;
                goto Label_Output;
            } else if (*psz == (TChar)'b')
            {
                psz++;
                uBase = 2;
            } else if (*psz == (TChar)'o')
            {
                psz++;
                uBase = 8;
            } else if (*psz == (TChar)'x')
            {
                psz++;
                uBase = 16;
            } else
            {
                uBase = 10;
            }
        } else
        {
            uBase = 10;
        }
    } else if (Base == 10 || Base == 16 || Base == 2 || Base == 8)
    {
        uBase = Base;
    } else
    {
        return FALSE;
    }

    // Overflow limitation
    if (ValueSize == sizeof(UINT8))
    {
        uMax = Unsigned ? UINT8_MAX : INT8_MAX;
    } else if (ValueSize == sizeof(UINT16))
    {
        uMax = Unsigned ? UINT16_MAX : INT16_MAX;
    } else if (ValueSize == sizeof(UINT32))
    {
        uMax = Unsigned ? UINT32_MAX : INT32_MAX;
    } else if (ValueSize == sizeof(UINT64))
    {
        uMax = Unsigned ? 0 : INT64_MAX;
    } else
    {
        return FALSE;
    }

    // Convert
    uTotal = 0;
    if (uBase == 2)
    {
        while ((ch = *psz++) != (TChar)'\0')
        {
            if (ch == (TChar)'0')
            {
                uTemp = uTotal << 1;
            } else if (ch == (TChar)'1')
            {
                uTemp = (uTotal << 1) | 1;
            } else
            {
                return FALSE;
            }
            if (uMax ? uTemp > uMax : uTemp <= uTotal)
            {
                return FALSE;
            }
            uTotal = uTemp;
        }
    } else if (uBase == 8)
    {
        while ((ch = *psz++) != (TChar)'\0')
        {
            if (ch >= (TChar)'0' && ch <= (TChar)'7')
            {
                uc = ch - (TChar)'0';
            } else
            {
                return FALSE;
            }
            uTemp = (uTotal << 3) + uc;
            if (uMax ? uTemp > uMax : uTemp <= uTotal)
            {
                return FALSE;
            }
            uTotal = uTemp;
        }
    } else if (uBase == 10)
    {
        while ((ch = *psz++) != (TChar)'\0')
        {
            if (ch >= (TChar)'0' && ch <= (TChar)'9')
            {
                uc = ch - (TChar)'0';
            } else
            {
                return FALSE;
            }
            uTemp = uTotal * 10 + uc;
            if (uMax ? uTemp > uMax : uTemp <= uTotal)
            {
                return FALSE;
            }
            uTotal = uTemp;
        }
    } else if (uBase == 16)
    {
        while ((ch = *psz++) != (TChar)'\0')
        {
            if (ch >= (TChar)'0' && ch <= (TChar)'9')
            {
                uc = ch - (TChar)'0';
            } else if (ch >= (TChar)'A' && ch <= (TChar)'F')
            {
                uc = ch - (TChar)'A' + 10;
            } else if (ch >= (TChar)'a' && ch <= (TChar)'f')
            {
                uc = ch - (TChar)'a' + 10;
            } else
            {
                return FALSE;
            }
            uTemp = (uTotal << 4) + uc;
            if (uMax ? uTemp > uMax : uTemp <= uTotal)
            {
                return FALSE;
            }
            uTotal = uTemp;
        }
    }

    // Output
    if (bMinus)
    {
        uTotal = -(INT64)uTotal;
    }
Label_Output:
    if (ValueSize == sizeof(UINT8))
    {
        *((PUINT8)Value) = (UINT8)uTotal;
    } else if (ValueSize == sizeof(UINT16))
    {
        *((PUINT16)Value) = (UINT16)uTotal;
    } else if (ValueSize == sizeof(UINT32))
    {
        *((PUINT32)Value) = (UINT32)uTotal;
    } else if (ValueSize == sizeof(UINT64))
    {
        *((PUINT64)Value) = (UINT64)uTotal;
    } else
    {
        return FALSE;
    }

    return TRUE;
}

_Success_(return != FALSE)
LOGICAL
NTAPI
Str_ToIntExW(
    _In_ PCWSTR StrValue,
    _In_ BOOL Unsigned,
    _In_ ULONG Base,
    _Out_writes_bytes_(ValueSize) PVOID Value,
    _In_ ULONG ValueSize)
{
    return Str_ToIntEx_Impl<WCHAR>(StrValue, Unsigned, Base, Value, ValueSize);
}

_Success_(return != FALSE)
LOGICAL
NTAPI
Str_ToIntExA(
    _In_ PCSTR StrValue,
    _In_ BOOL Unsigned,
    _In_ ULONG Base,
    _Out_writes_bytes_(ValueSize) PVOID Value,
    _In_ ULONG ValueSize)
{
    return Str_ToIntEx_Impl<CHAR>(StrValue, Unsigned, Base, Value, ValueSize);
}

template <typename TChar>
static
_Success_(return > 0)
ULONG
NTAPI
Str_FromIntEx_Impl(
    _In_ INT64 Value,
    _In_ BOOL Unsigned,
    _In_ UINT Base,
    _Out_writes_(DestCchSize) TChar* StrValue,
    _In_ ULONG DestCchSize)
{
    TChar* psz = StrValue;
    UINT64 uTotal, uDivisor, uPowerFlag;
    LOGICAL lRet;
    BOOL bNegative = !Unsigned && Value < 0;
    UINT64 uDivisorTemp = Base;

    // Minus
    uTotal = bNegative ? -Value : (UINT64)Value;

    // Base
    if (Base == 2)
    {
        uPowerFlag = 1;
    } else if (Base == 8)
    {
        uPowerFlag = 3;
    } else if (Base == 10)
    {
        uPowerFlag = 0;
    } else if (Base == 16)
    {
        uPowerFlag = 4;
    } else
    {
        return 0;
    }

    if (DestCchSize <= 1)
    {
        return 0;
    }

    // Find max divisor
    do
    {
        uDivisor = uDivisorTemp;
        uDivisorTemp = uPowerFlag ? uDivisorTemp << uPowerFlag : uDivisorTemp * 10;
    } while (uTotal >= uDivisorTemp && uDivisorTemp > uDivisor);

    // Convert
    lRet = TRUE;
    if (bNegative)
    {
        *psz++ = (TChar)'-';
    }
    while (TRUE)
    {
        UINT64 i = uTotal / uDivisor;
        uTotal = uTotal % uDivisor;
        if ((ULONG_PTR)psz - (ULONG_PTR)StrValue < ((ULONG_PTR)DestCchSize - 1) * sizeof(TChar))
        {
            if (i != 0 || (bNegative ? psz != StrValue + 1 : psz != StrValue))
            {
                *psz++ = (TChar)(i <= 9 ? i + (TChar)'0' : i - 10 + (TChar)'A');
            }
        } else
        {
            lRet = FALSE;
            break;
        }
        if (uDivisor == Base)
        {
            i = uTotal;
            if ((ULONG_PTR)psz - (ULONG_PTR)StrValue < ((ULONG_PTR)DestCchSize - 1) * sizeof(TChar))
            {
                *psz++ = (TChar)(i <= 9 ? i + (TChar)'0' : i - 10 + (TChar)'A');
            } else
            {
                lRet = FALSE;
            }
            break;
        }
        uDivisor = uPowerFlag ? uDivisor >> uPowerFlag : uDivisor / 10;
    };
    *psz = (TChar)'\0';

    return PtrOffset(StrValue, psz) / sizeof(TChar);
}

_Success_(return > 0)
ULONG
NTAPI
Str_FromIntExW(
    _In_ INT64 Value,
    _In_ BOOL Unsigned,
    _In_ UINT Base,
    _Out_writes_(DestCchSize) PWSTR StrValue,
    _In_ ULONG DestCchSize)
{
    return Str_FromIntEx_Impl<WCHAR>(Value, Unsigned, Base, StrValue, DestCchSize);
}

_Success_(return > 0)
ULONG
NTAPI
Str_FromIntExA(
    _In_ INT64 Value,
    _In_ BOOL Unsigned,
    _In_ UINT Base,
    _Out_writes_(DestCchSize) PSTR StrValue,
    _In_ ULONG DestCchSize)
{
    return Str_FromIntEx_Impl<CHAR>(Value, Unsigned, Base, StrValue, DestCchSize);
}
