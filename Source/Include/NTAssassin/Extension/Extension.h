#pragma once

#include "../NT/MinDef.h"

#ifndef _NTASSASSIN_NTDEF_NO_EXTENSION_CPU_
#include "./CPU.h"
#endif

#ifndef _NTASSASSIN_NTDEF_NO_EXTENSION_MSTOOLCHAIN_
#include "./MSToolChain.h"
#endif

#ifndef _NTASSASSIN_NTDEF_NO_EXTENSION_SMBIOS_
#include "./SMBIOS.h"
#endif

// Gets equality of two value after masked
#define IS_EQUAL_MASKED(val1, val2, mask) (!(((val1) ^ (val2)) & (mask)))
// Sets or removes a flag from a combination value
#define COMBINE_FLAGS(val, uflag, bEnable) ((bEnable) ? ((val) | (uflag)) : ((val) & ~(uflag)))
// Test combined flags
#define TEST_FLAGS(val, flags) (((val) & (flags)) == (flags))

// Gets is the value is within the valid range of an atom
#define IS_ATOM(val) (((ULONG_PTR)(val) & 0xFFFF) > 0 && ((ULONG_PTR)(val) & 0xFFFF) < MAXINTATOM)

#pragma region Size in bytes

#define BYTE_BIT 8UL
#define KB_TO_BYTES(x) ((x) * 1024UL)
#define MB_TO_KB(x) ((x) * 1024UL)
#define MB_TO_BYTES(x) (KB_TO_BYTES(MB_TO_KB(x)))
#define GB_TO_MB(x) ((x) * 1024UL)
#define GB_TO_BYTES(x) (MB_TO_BYTES(GB_TO_MB(x)))
#define TB_TO_GB(x) ((x) * 1024UL)
#define TB_TO_BYTES(x) (GB_TO_BYTES(TB_TO_GB(x)))

#pragma endregion

#pragma region Limitations

#define MAX_CLASSNAME_CCH       256
#define MAX_CIDENTIFIERNAME_CCH 247
#define MAX_ATOM_CCH            255
#define MAX_REG_KEYNAME_CCH     255
#define MAX_REG_VALUENAME_CCH   16383
#define POINTER_CCH             (sizeof(PVOID) * 2 + 1)
#define HEX_RGB_CCH             8 // #RRGGBB

#pragma endregion

#pragma region Alignments

#define CODE_ALIGNMENT 0x10
#define STRING_ALIGNMENT 0x4

#pragma endregion

#pragma region String

#define _STR_CCH_LEN(quote) (ARRAYSIZE(quote) - 1)

#define ASCII_CASE_MASK 0b100000
#define UNICODE_EOL ((DWORD)0x000A000D)
#define ANSI_EOL ((WORD)0x0A0D)

#pragma endregion

#pragma region Any-size array

#define ANYSIZE_STRUCT_SIZE(structure, field, size) UFIELD_OFFSET(structure, field[size])

#define DEFINE_ANYSIZE_STRUCT(varname, structure, type, size) struct {\
    structure Base;\
    type Extra[(size) - 1];\
} varname

#pragma endregion
