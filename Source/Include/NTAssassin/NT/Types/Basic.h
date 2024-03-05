#pragma once

#include "../MinDef.h"

typedef enum _ALTERNATIVE_ARCHITECTURE_TYPE
{
    StandardDesign,                 // None == 0 == standard design
    NEC98x86,                       // NEC PC98xx series on X86
    EndAlternatives                 // past end of known alternatives
} ALTERNATIVE_ARCHITECTURE_TYPE;

typedef struct _LEAP_SECOND_DATA
{
    UCHAR Enabled;
    UCHAR Padding[3];
    ULONG Count;
    _Field_size_(Count) LARGE_INTEGER Data[ANYSIZE_ARRAY];
} LEAP_SECOND_DATA, *PLEAP_SECOND_DATA;

typedef struct _ACTIVATION_CONTEXT_DATA
{
    ULONG Magic;
    ULONG HeaderSize;
    ULONG FormatVersion;
    ULONG TotalSize;
    ULONG DefaultTocOffset;
    ULONG ExtendedTocOffset;
    ULONG AssemblyRosterOffset;
    ULONG Flags;
} ACTIVATION_CONTEXT_DATA, *PACTIVATION_CONTEXT_DATA;

#pragma region CLIENT_ID[64/32]

typedef struct _CLIENT_ID
{
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

#if defined(_NTASSASSIN_NDK_EXTENSION_)

typedef struct _CLIENT_ID64
{
    VOID* POINTER_64 UniqueProcess;
    VOID* POINTER_64 UniqueThread;
} CLIENT_ID64, *PCLIENT_ID64;

typedef struct _CLIENT_ID32
{
    VOID* POINTER_32 UniqueProcess;
    VOID* POINTER_32 UniqueThread;
} CLIENT_ID32, *PCLIENT_ID32;

#endif /* defined(_NTASSASSIN_NDK_EXTENSION_) */

#pragma endregion

#pragma region CURDIR[64/32]

typedef struct _CURDIR
{
    UNICODE_STRING DosPath;
    HANDLE Handle;
} CURDIR, *PCURDIR;

#if defined(_NTASSASSIN_NDK_EXTENSION_)

typedef struct _CURDIR64
{
    UNICODE_STRING64 DosPath;
    VOID* POINTER_64 Handle;
} CURDIR64, *PCURDIR64;

typedef struct _CURDIR32
{
    UNICODE_STRING32 DosPath;
    VOID* POINTER_32 Handle;
} CURDIR32, *PCURDIR32;

#endif /* defined(_NTASSASSIN_NDK_EXTENSION_) */

#pragma endregion

#pragma region ASSEMBLY_STORAGE_MAP_ENTRY[64/32]

typedef struct _ASSEMBLY_STORAGE_MAP_ENTRY
{
    ULONG Flags;
    UNICODE_STRING DosPath;
    HANDLE Handle;
} ASSEMBLY_STORAGE_MAP_ENTRY, *PASSEMBLY_STORAGE_MAP_ENTRY;

#if defined(_NTASSASSIN_NDK_EXTENSION_)

typedef struct _ASSEMBLY_STORAGE_MAP_ENTRY64
{
    ULONG Flags;
    UNICODE_STRING64 DosPath;
    VOID* POINTER_64 Handle;
} ASSEMBLY_STORAGE_MAP_ENTRY64, *PASSEMBLY_STORAGE_MAP_ENTRY64;

typedef struct _ASSEMBLY_STORAGE_MAP_ENTRY32
{
    ULONG Flags;
    UNICODE_STRING32 DosPath;
    VOID* POINTER_32 Handle;
} ASSEMBLY_STORAGE_MAP_ENTRY32, *PASSEMBLY_STORAGE_MAP_ENTRY32;

#endif /* defined(_NTASSASSIN_NDK_EXTENSION_) */

#pragma endregion

#pragma region ASSEMBLY_STORAGE_MAP[64/32]

typedef struct _ASSEMBLY_STORAGE_MAP
{
    ULONG Flags;
    ULONG AssemblyCount;
    PASSEMBLY_STORAGE_MAP_ENTRY* AssemblyArray;
} ASSEMBLY_STORAGE_MAP, *PASSEMBLY_STORAGE_MAP;

#if defined(_NTASSASSIN_NDK_EXTENSION_)

typedef struct _ASSEMBLY_STORAGE_MAP64
{
    ULONG Flags;
    ULONG AssemblyCount;
    ASSEMBLY_STORAGE_MAP_ENTRY64* POINTER_64* AssemblyArray;
} ASSEMBLY_STORAGE_MAP64, *PASSEMBLY_STORAGE_MAP64;

typedef struct _ASSEMBLY_STORAGE_MAP32
{
    ULONG Flags;
    ULONG AssemblyCount;
    ASSEMBLY_STORAGE_MAP_ENTRY32* POINTER_32* AssemblyArray;
} ASSEMBLY_STORAGE_MAP32, *PASSEMBLY_STORAGE_MAP32;

#endif /* defined(_NTASSASSIN_NDK_EXTENSION_) */

#pragma endregion

#pragma region [RTL_]ACTIVATION_CONTEXT_STACK[_FRAME][64/32]

typedef struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME RTL_ACTIVATION_CONTEXT_STACK_FRAME, *PRTL_ACTIVATION_CONTEXT_STACK_FRAME;
typedef struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME64 RTL_ACTIVATION_CONTEXT_STACK_FRAME64, *PRTL_ACTIVATION_CONTEXT_STACK_FRAME64;
typedef struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME32 RTL_ACTIVATION_CONTEXT_STACK_FRAME32, *PRTL_ACTIVATION_CONTEXT_STACK_FRAME32;

struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME
{
    struct RTL_ACTIVATION_CONTEXT_STACK_FRAME* Previous;
    struct ACTIVATION_CONTEXT* ActivationContext;
    DWORD Flags;
};

struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME64
{
    struct RTL_ACTIVATION_CONTEXT_STACK_FRAME* POINTER_64 Previous;
    struct ACTIVATION_CONTEXT* POINTER_64 ActivationContext;
    ULONG Flags;
};

struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME32
{
    struct RTL_ACTIVATION_CONTEXT_STACK_FRAME* POINTER_32 Previous;
    struct ACTIVATION_CONTEXT* POINTER_32 ActivationContext;
    ULONG Flags;
};

typedef struct _ACTIVATION_CONTEXT_STACK
{
    PRTL_ACTIVATION_CONTEXT_STACK_FRAME ActiveFrame;
    LIST_ENTRY FrameListCache;
    ULONG Flags;
    ULONG NextCookieSequenceNumber;
    ULONG StackId;
} ACTIVATION_CONTEXT_STACK, *PACTIVATION_CONTEXT_STACK;

typedef struct _ACTIVATION_CONTEXT_STACK64
{
    RTL_ACTIVATION_CONTEXT_STACK_FRAME64* POINTER_64 ActiveFrame;
    LIST_ENTRY64 FrameListCache;
    ULONG Flags;
    ULONG NextCookieSequenceNumber;
    ULONG StackId;
} ACTIVATION_CONTEXT_STACK64, *PACTIVATION_CONTEXT_STACK64;

typedef struct _ACTIVATION_CONTEXT_STACK32
{
    RTL_ACTIVATION_CONTEXT_STACK_FRAME32* POINTER_32 ActiveFrame;
    LIST_ENTRY32 FrameListCache;
    ULONG Flags;
    ULONG NextCookieSequenceNumber;
    ULONG StackId;
} ACTIVATION_CONTEXT_STACK32, *PACTIVATION_CONTEXT_STACK32;

#pragma endregion

#pragma region GDI_TEB_BATCH[64/32]

typedef struct _GDI_TEB_BATCH
{
    struct
    {
        ULONG Offset : 31;
        BOOL HasRenderingCommand : 1;
    };
    ULONG_PTR HDC;
    ULONG Buffer[310];
} GDI_TEB_BATCH, *PGDI_TEB_BATCH;

#if defined(_NTASSASSIN_NDK_EXTENSION_)

typedef struct _GDI_TEB_BATCH64
{
    struct
    {
        ULONG Offset : 31;
        BOOL HasRenderingCommand : 1;
    };
    ULONGLONG HDC;
    ULONG Buffer[310];
} GDI_TEB_BATCH64, *PGDI_TEB_BATCH64;

typedef struct _GDI_TEB_BATCH32
{
    struct
    {
        ULONG Offset : 31;
        BOOL HasRenderingCommand : 1;
    };
    ULONG HDC;
    ULONG Buffer[310];
} GDI_TEB_BATCH32, *PGDI_TEB_BATCH32;

#endif /* defined(_NTASSASSIN_NDK_EXTENSION_) */

#pragma endregion
