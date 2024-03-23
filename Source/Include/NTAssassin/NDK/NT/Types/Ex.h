#pragma once

#include "../MinDef.h"

#pragma region Atom

typedef enum _ATOM_INFORMATION_CLASS
{
    AtomBasicInformation,
    AtomTableInformation,
} ATOM_INFORMATION_CLASS, *PATOM_INFORMATION_CLASS;

typedef struct _ATOM_BASIC_INFORMATION
{
    USHORT UsageCount;
    USHORT Flags;
    USHORT NameLength;
    _Field_size_(NameLength) WCHAR Name[ANYSIZE_ARRAY];
} ATOM_BASIC_INFORMATION, *PATOM_BASIC_INFORMATION;

typedef struct _ATOM_TABLE_INFORMATION
{
    ULONG NumberOfAtoms;
    _Field_size_(NumberOfAtoms) USHORT Atoms[ANYSIZE_ARRAY];
} ATOM_TABLE_INFORMATION, *PATOM_TABLE_INFORMATION;

typedef USHORT RTL_ATOM, *PRTL_ATOM;

#pragma endregion

#pragma region Event

typedef enum _EVENT_INFORMATION_CLASS
{
    EventBasicInformation
} EVENT_INFORMATION_CLASS;

typedef struct _EVENT_BASIC_INFORMATION
{
    EVENT_TYPE EventType;
    LONG EventState;
} EVENT_BASIC_INFORMATION, *PEVENT_BASIC_INFORMATION;

#pragma endregion

#pragma region Firmware variable attributes

#define VARIABLE_ATTRIBUTE_NON_VOLATILE                             0x00000001
#define VARIABLE_ATTRIBUTE_BOOTSERVICE_ACCESS                       0x00000002
#define VARIABLE_ATTRIBUTE_RUNTIME_ACCESS                           0x00000004
#define VARIABLE_ATTRIBUTE_HARDWARE_ERROR_RECORD                    0x00000008
#define VARIABLE_ATTRIBUTE_AUTHENTICATED_WRITE_ACCESS               0x00000010
#define VARIABLE_ATTRIBUTE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS    0x00000020
#define VARIABLE_ATTRIBUTE_APPEND_WRITE                             0x00000040

#pragma endregion
