#include "Test.h"

typedef struct _NT_PATH_TEST
{
    typeof(&RtlDosLongPathNameToNtPathName_U_WithStatus) SystemFunction;
    ULONG Comparisons;
    ULONG Failures;
} NT_PATH_TEST;

static
VOID
TestNtPathCase(
    _Inout_ NT_PATH_TEST* Test,
    _In_ PCWSTR Path)
{
    WCHAR Sentinel = L'!';
    UNICODE_STRING Expected = { 0xAAAA, 0xBBBB, &Sentinel };
    UNICODE_STRING Actual = Expected;
    NTSTATUS ExpectedStatus, Status;
    BOOLEAN Equal;

    ExpectedStatus = Test->SystemFunction(Path, &Expected, NULL, NULL);
    Status = NT_Win32PathToNtPath(Path, &Actual);
    Equal = Status == ExpectedStatus &&
            Actual.Length == Expected.Length && Actual.MaximumLength == Expected.MaximumLength;
    if (NT_SUCCESS(ExpectedStatus) && NT_SUCCESS(Status))
    {
        Equal = Equal && Actual.Buffer != NULL && Expected.Buffer != NULL &&
                RtlEqualMemory(Actual.Buffer, Expected.Buffer, Expected.Length + sizeof(WCHAR));
    } else
    {
        Equal = Equal && Actual.Buffer == Expected.Buffer;
    }
    Test->Comparisons++;
    if (!Equal)
    {
        Test->Failures++;
        if (Test->Failures <= 16)
        {
            UnitTest_PrintF("status %08lX/%08lX, length %u/%u, capacity %u/%u, input %.80ls\n",
                            (ULONG)Status,
                            (ULONG)ExpectedStatus,
                            Actual.Length,
                            Expected.Length,
                            Actual.MaximumLength,
                            Expected.MaximumLength,
                            Path);
        }
    }
    if (NT_SUCCESS(Status))
    {
        NT_FreeNtPath(&Actual);
    }
    if (NT_SUCCESS(ExpectedStatus))
    {
        NT_FreeNtPath(&Expected);
    }
}

TEST_FUNC(NT_Win32LongPath)
{
    static const PCWSTR Cases[] = {
        L"", L" ", L".", L"..", L"\\", L"/", L"C:", L"C:\\", L"c:/",
        L"relative.txt", L"..\\a\\.\\b\\..\\c", L"C:relative.txt", L"C:\\a\\..\\b",
        L"C:\\a. \\b... ", L"C:\\a\\\\b/./c", L"C:\\中文\\\xD83D\xDE00.txt",
        L"\\\\server\\share", L"\\\\server\\share\\a\\..\\b", L"//server/share/a/../b",
        L"\\\\server", L"\\\\", L"\\\\.\\", L"\\\\.\\C:\\a\\..\\b", L"\\\\.\\UNC\\server\\share\\a",
        L"\\\\?\\", L"\\\\?\\C:\\a\\..\\b. ", L"\\\\?\\UNC\\server\\share\\a",
        L"\\??\\C:\\a\\..\\b", L"NUL", L"nul.txt", L"CON", L"COM1:", L"LPT9.txt", L"CONIN$",
        L"C:\\NUL", L"C:\\FWI-path-test-missing-parent\\NUL", L"C:\\a:b", L"C:\\?*<>|",
        L"Z:relative.txt", L"C:\\a\\\xD800.txt", L"C:\\a\\\xDC00.txt"
    };
    static const PCWSTR Prefixes[] = {
        L"C:\\", L"C:", L"\\", L"", L"\\\\server\\share\\", L"\\\\.\\C:\\",
        L"\\\\?\\C:\\", L"\\\\?\\UNC\\server\\share\\", L"\\??\\C:\\", L"C:/", L"//server/share/"
    };
    static const ULONG Lengths[] = {
        0, 1, 2, 3, 4, 5, 8, 16, 255, 256, 257, 258, 259, 260, 261, 262, 263,
        511, 512, 1023, 4095, 16384, 32730, 32755, 32758, 32759, 32760, 32761,
        32762, 32763, 32764, 32765, 32766, 32767, 32768
    };
    static const WCHAR Alphabet[] = L"abCD09_ .\\/:\x4E2D\x6587\xD83D\xDE00";
    static const WCHAR Segment[] = L"segment\\..\\";
    NT_PATH_TEST Test = {
        .SystemFunction = (typeof(&RtlDosLongPathNameToNtPathName_U_WithStatus))GetProcAddress(
            GetModuleHandleW(L"ntdll.dll"),
            "RtlDosLongPathNameToNtPathName_U_WithStatus")
    };
    PWSTR Path;
    ULONG i, j, k, Length, PrefixLength, Seed = 0x12345678;
    UNICODE_STRING NtPath;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(TEST_PARAMETER_ARGC);
    UNREFERENCED_PARAMETER(TEST_PARAMETER_ARGV);
    Path = Mem_Alloc(34000 * sizeof(WCHAR));
    TEST_OK(Path != NULL);
    if (Path == NULL)
    {
        return;
    }

    Str_CopyExW(Path, 34000, L"C:\\");
    for (i = 3; i < 600; i++)
    {
        Path[i] = L'a';
    }
    Path[600] = UNICODE_NULL;
    Status = NT_Win32PathToNtPath(Path, &NtPath);
    TEST_OK(NT_SUCCESS(Status));
    if (NT_SUCCESS(Status))
    {
        TEST_OK(NtPath.Length == 604 * sizeof(WCHAR));
        TEST_OK(NtPath.MaximumLength == NtPath.Length + sizeof(WCHAR));
        TEST_OK(RtlEqualMemory(NtPath.Buffer, L"\\??\\", 4 * sizeof(WCHAR)));
        TEST_OK(RtlEqualMemory(NtPath.Buffer + 4, Path, 601 * sizeof(WCHAR)));
        TEST_OK(NT_FreeNtPath(&NtPath));
    }
    Status = RtlDosPathNameToNtPathName_U_WithStatus(Path, &NtPath, NULL, NULL);
    UnitTest_PrintF("Legacy RTL status for a 600-character DOS path: %08lX\n", (ULONG)Status);
    if (NT_SUCCESS(Status))
    {
        NT_FreeNtPath(&NtPath);
    }
    if (Test.SystemFunction == NULL)
    {
        TEST_SKIP("The native long-path export is unavailable for differential testing.");
        Mem_Free(Path);
        return;
    }

    for (i = 0; i < ARRAYSIZE(Cases); i++)
    {
        TestNtPathCase(&Test, Cases[i]);
    }
    for (i = 0; i < ARRAYSIZE(Prefixes); i++)
    {
        PrefixLength = (ULONG)wcslen(Prefixes[i]);
        for (j = 0; j < ARRAYSIZE(Lengths); j++)
        {
            Length = Lengths[j];
            if (Length < PrefixLength)
            {
                continue;
            }
            RtlCopyMemory(Path, Prefixes[i], PrefixLength * sizeof(WCHAR));
            for (k = PrefixLength; k < Length; k++)
            {
                Path[k] = L'a';
            }
            Path[Length] = UNICODE_NULL;
            TestNtPathCase(&Test, Path);
            if (Length > PrefixLength)
            {
                Path[Length - 1] = L' ';
                TestNtPathCase(&Test, Path);
                Path[Length - 1] = L'.';
                TestNtPathCase(&Test, Path);
                if (Length >= PrefixLength + 4)
                {
                    RtlCopyMemory(Path + Length - 4, L"\\..\\", 4 * sizeof(WCHAR));
                    TestNtPathCase(&Test, Path);
                }
            }
        }
    }
    for (i = 1; i <= 2800; i += 37)
    {
        Path[0] = L'C';
        Path[1] = L':';
        Path[2] = L'\\';
        for (j = 0; j < i; j++)
        {
            RtlCopyMemory(Path + 3 + j * (ARRAYSIZE(Segment) - 1), Segment, sizeof(Segment) - sizeof(WCHAR));
        }
        Length = 3 + i * (ARRAYSIZE(Segment) - 1);
        Path[Length] = UNICODE_NULL;
        TestNtPathCase(&Test, Path);
    }
    for (i = 0; i < 20000; i++)
    {
        j = RtlRandomEx(&Seed) % ARRAYSIZE(Prefixes);
        PrefixLength = (ULONG)wcslen(Prefixes[j]);
        RtlCopyMemory(Path, Prefixes[j], PrefixLength * sizeof(WCHAR));
        Length = PrefixLength + RtlRandomEx(&Seed) % 2048;
        for (k = PrefixLength; k < Length; k++)
        {
            Path[k] = Alphabet[RtlRandomEx(&Seed) % (ARRAYSIZE(Alphabet) - 1)];
        }
        Path[Length] = UNICODE_NULL;
        TestNtPathCase(&Test, Path);
    }
    TEST_OK(Test.Failures == 0);
    UnitTest_PrintF("Native path comparisons: %lu, mismatches: %lu\n", Test.Comparisons, Test.Failures);
    Mem_Free(Path);
}

static
VOID
TestNtWin32Object(
    _Inout_ PUNITTEST_RESULT TEST_PARAMETER_RESULT,
    _In_ PCWSTR Path)
{
    WCHAR Sentinel = L'!';
    UNICODE_STRING Expected = { 0xAAAA, 0xBBBB, &Sentinel };
    UNICODE_STRING Name = Expected;
    OBJECT_ATTRIBUTES Object, Original;
    NTSTATUS Status, ExpectedStatus;

    RtlFillMemory(&Object, sizeof(Object), 0xA5);
    Original = Object;
    ExpectedStatus = NT_Win32PathToNtPath(Path, &Expected);
    Status = NT_InitWin32PathObject(&Object, Path, &Name);
    TEST_OK(Status == ExpectedStatus);
    if (NT_SUCCESS(Status) && NT_SUCCESS(ExpectedStatus))
    {
        TEST_OK(Object.Length == sizeof(Object) && Object.ObjectName == &Name && Object.RootDirectory == NULL &&
                Object.Attributes == OBJ_CASE_INSENSITIVE && Object.SecurityDescriptor == NULL &&
                Object.SecurityQualityOfService == NULL);
        TEST_OK(Name.Length == Expected.Length && Name.MaximumLength == Expected.MaximumLength &&
                Name.Buffer != NULL && Name.Buffer != &Sentinel &&
                RtlEqualMemory(Name.Buffer, Expected.Buffer, Expected.Length + sizeof(WCHAR)));
    } else if (!NT_SUCCESS(Status) && !NT_SUCCESS(ExpectedStatus))
    {
        TEST_OK(Name.Buffer == Expected.Buffer && Name.Length == Expected.Length &&
                Name.MaximumLength == Expected.MaximumLength);
        TEST_OK(RtlEqualMemory(&Object, &Original, sizeof(Object)));
    }
    if (NT_SUCCESS(Status) && Name.Buffer != NULL && Name.Buffer != &Sentinel)
    {
        TEST_OK(NT_FreeNtPath(&Name));
    }
    if (NT_SUCCESS(ExpectedStatus) && Expected.Buffer != NULL && Expected.Buffer != &Sentinel)
    {
        TEST_OK(NT_FreeNtPath(&Expected));
    }
}

static
BOOLEAN
TestNtBorrowedName(
    _Inout_ PUNITTEST_RESULT TEST_PARAMETER_RESULT,
    _In_ PCWSTR Path,
    _Out_ PUNICODE_STRING Name)
{
    NTSTATUS Status = _Inline_RtlInitUnicodeStringEx(Name, Path);

    TEST_OK(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }
    TEST_OK(Name->Buffer == Path);
    return TRUE;
}

static
BOOLEAN
TestNtRootFile(
    _Inout_ PUNITTEST_RESULT TEST_PARAMETER_RESULT,
    _In_ HANDLE Root,
    _In_ PCWSTR Path,
    _In_ BOOLEAN Stream)
{
    static const CHAR Data[] = "Root";
    const ULONG Share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    HANDLE File = NULL;
    FILE_NETWORK_OPEN_INFORMATION Attributes, NativeAttributes;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES Object;
    CHAR Buffer[sizeof(Data) - 1];
    ULONG Bytes;
    NTSTATUS Status, NativeStatus;
    BOOLEAN Created = FALSE, Success = FALSE;

    if (!TestNtBorrowedName(TEST_PARAMETER_RESULT, Path, &Name))
    {
        return FALSE;
    }
    Status = IO_CreateFile(&File,
                           &Name,
                           Root,
                           FILE_READ_DATA | FILE_WRITE_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                           Share,
                           FILE_CREATE,
                           FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    TEST_OK(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }
    Created = TRUE;
    Status = IO_WriteFile(File, NULL, (PVOID)Data, sizeof(Data) - 1, &Bytes);
    TEST_OK(NT_SUCCESS(Status) && Bytes == sizeof(Data) - 1);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    TEST_OK(NT_SUCCESS(NtClose(File)));
    File = NULL;
    Status = IO_OpenFile(&File, &Name, Root, FILE_READ_DATA | SYNCHRONIZE, Share);
    TEST_OK(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
    {
        File = NULL;
        goto Cleanup;
    }
    Status = IO_ReadFile(File, NULL, Buffer, sizeof(Buffer), &Bytes);
    TEST_OK(NT_SUCCESS(Status) && Bytes == sizeof(Buffer) && RtlEqualMemory(Buffer, Data, sizeof(Buffer)));
    TEST_OK(NT_SUCCESS(NtClose(File)));
    File = NULL;
    if (Stream)
    {
        NT_InitObject(&Object, &Name, OBJ_CASE_INSENSITIVE, Root);
        NativeStatus = NtQueryFullAttributesFile(&Object, &NativeAttributes);
        Status = IO_GetFileAttributes(&Name, Root, &Attributes);
        TEST_OK(Status == NativeStatus);
        if (NT_SUCCESS(Status) && NT_SUCCESS(NativeStatus))
        {
            TEST_OK(Attributes.EndOfFile.QuadPart == NativeAttributes.EndOfFile.QuadPart &&
                    Attributes.AllocationSize.QuadPart == NativeAttributes.AllocationSize.QuadPart &&
                    Attributes.FileAttributes == NativeAttributes.FileAttributes);
        }
    } else
    {
        Status = IO_GetFileAttributes(&Name, Root, &Attributes);
        TEST_OK(NT_SUCCESS(Status) && Attributes.EndOfFile.QuadPart == sizeof(Data) - 1 &&
                !BooleanFlagOn(Attributes.FileAttributes, FILE_ATTRIBUTE_DIRECTORY));
    }
    Status = IO_DeleteFile(&Name, Root);
    TEST_OK(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    Created = FALSE;
    Status = IO_OpenFile(&File, &Name, Root, FILE_READ_DATA | SYNCHRONIZE, Share);
    TEST_OK(Status == STATUS_OBJECT_NAME_NOT_FOUND);
    Success = Status == STATUS_OBJECT_NAME_NOT_FOUND;
    if (NT_SUCCESS(Status))
    {
        Created = TRUE;
    } else
    {
        File = NULL;
    }
Cleanup:
    if (File != NULL)
    {
        TEST_OK(NT_SUCCESS(NtClose(File)));
    }
    if (Created)
    {
        TEST_OK(NT_SUCCESS(IO_DeleteFile(&Name, Root)));
    }
    return Success;
}

static
BOOLEAN
TestNtWin32LongFile(
    _Inout_ PUNITTEST_RESULT TEST_PARAMETER_RESULT,
    _In_ HANDLE Root,
    _In_ PCWSTR DirectoryPath)
{
    static const CHAR Data[] = "Long";
    const ULONG Share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    WCHAR Leaf[251], Path[MAX_PATH + 256];
    SIZE_T DirectoryLength = wcslen(DirectoryPath);
    UNICODE_STRING Name;
    FILE_NETWORK_OPEN_INFORMATION Attributes;
    HANDLE File = NULL;
    CHAR Buffer[sizeof(Data) - 1];
    ULONG i, Bytes;
    NTSTATUS Status;
    BOOLEAN Created = FALSE, Success = FALSE;

    for (i = 0; i < ARRAYSIZE(Leaf) - 1; i++)
    {
        Leaf[i] = L'a';
    }
    Leaf[ARRAYSIZE(Leaf) - 1] = UNICODE_NULL;
    RtlCopyMemory(Path, DirectoryPath, DirectoryLength * sizeof(WCHAR));
    Path[DirectoryLength] = L'\\';
    RtlCopyMemory(Path + DirectoryLength + 1, Leaf, sizeof(Leaf));
    TEST_OK(wcslen(Path) > MAX_PATH);
    if (!TestNtBorrowedName(TEST_PARAMETER_RESULT, Leaf, &Name))
    {
        return FALSE;
    }
    Status = IO_CreateWin32File(&File,
                                Path,
                                FILE_READ_DATA | FILE_WRITE_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                                Share,
                                FILE_CREATE,
                                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    TEST_OK(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }
    Created = TRUE;
    Status = IO_WriteFile(File, NULL, (PVOID)Data, sizeof(Data) - 1, &Bytes);
    TEST_OK(NT_SUCCESS(Status) && Bytes == sizeof(Data) - 1);
    TEST_OK(NT_SUCCESS(NtClose(File)));
    File = NULL;
    Status = IO_OpenWin32File(&File, Path, FILE_READ_DATA | SYNCHRONIZE, Share);
    TEST_OK(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
    {
        File = NULL;
        goto Cleanup;
    }
    Status = IO_ReadFile(File, NULL, Buffer, sizeof(Buffer), &Bytes);
    TEST_OK(NT_SUCCESS(Status) && Bytes == sizeof(Buffer) && RtlEqualMemory(Buffer, Data, sizeof(Buffer)));
    TEST_OK(NT_SUCCESS(NtClose(File)));
    File = NULL;
    Status = IO_GetWin32FileAttributes(Path, &Attributes);
    TEST_OK(NT_SUCCESS(Status) && Attributes.EndOfFile.QuadPart == sizeof(Data) - 1);
    Status = IO_GetFileAttributes(&Name, Root, &Attributes);
    TEST_OK(NT_SUCCESS(Status) && Attributes.EndOfFile.QuadPart == sizeof(Data) - 1);
    Status = IO_DeleteWin32File(Path);
    TEST_OK(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    Created = FALSE;
    Status = IO_OpenWin32File(&File, Path, FILE_READ_DATA | SYNCHRONIZE, Share);
    TEST_OK(Status == STATUS_OBJECT_NAME_NOT_FOUND);
    Success = Status == STATUS_OBJECT_NAME_NOT_FOUND;
    if (NT_SUCCESS(Status))
    {
        Created = TRUE;
    } else
    {
        File = NULL;
    }
Cleanup:
    if (File != NULL)
    {
        TEST_OK(NT_SUCCESS(NtClose(File)));
    }
    if (Created)
    {
        TEST_OK(NT_SUCCESS(IO_DeleteFile(&Name, Root)));
    }
    return Success;
}

TEST_FUNC(NT_NativeRootDirectory)
{
    static const PCWSTR Win32Names[] = { L"C:\\root\\child", L"sub/../file.txt", L"NUL", L"" };
    static const PCWSTR NativeFiles[] = { L"NUL", L"CON", L"COM1" };
    const ULONG Share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    const ACCESS_MASK DirectoryAccess = FILE_LIST_DIRECTORY | FILE_ADD_FILE | FILE_ADD_SUBDIRECTORY |
                                       FILE_READ_ATTRIBUTES | DELETE | SYNCHRONIZE;
    static const CHAR FileData[] = "Root";
    WCHAR TempPath[MAX_PATH], FixturePath[MAX_PATH];
    PWSTR SavedDirectory = NULL;
    HANDLE Root = NULL, Sub = NULL, Nested = NULL, File = NULL;
    FILE_NETWORK_OPEN_INFORMATION Attributes, NativeAttributes;
    UNICODE_STRING EmptyName, BaseName, SubName, NestedName, FileName, MissingName, DotName, SlashName, AbsoluteName;
    UNICODE_STRING FixtureNtName = { 0 };
    OBJECT_ATTRIBUTES DotObject;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE NativeFile;
    NTSTATUS NativeStatus;
    CHAR ReadData[sizeof(FileData) - 1];
    ULONG i, Bytes;
    DWORD Count, SavedCount, FileSystemFlags, MaximumComponentLength;
    NTSTATUS Status;
    BOOL Restored, VolumeKnown;
    BOOLEAN TempFileCreated = FALSE, FixtureCreated = FALSE, FileCreated = FALSE, DirectoryChanged = FALSE;
    BOOLEAN BaseCreated = FALSE;

    UNREFERENCED_PARAMETER(TEST_PARAMETER_ARGC);
    UNREFERENCED_PARAMETER(TEST_PARAMETER_ARGV);
    for (i = 0; i < ARRAYSIZE(Win32Names); i++)
    {
        TestNtWin32Object(TEST_PARAMETER_RESULT, Win32Names[i]);
    }
    if (!TestNtBorrowedName(TEST_PARAMETER_RESULT, L"", &EmptyName) ||
        !TestNtBorrowedName(TEST_PARAMETER_RESULT, L"C", &BaseName) ||
        !TestNtBorrowedName(TEST_PARAMETER_RESULT, L"sub", &SubName) ||
        !TestNtBorrowedName(TEST_PARAMETER_RESULT, L"sub\\nested", &NestedName) ||
        !TestNtBorrowedName(TEST_PARAMETER_RESULT, L"sub\\nested\\file.txt", &FileName) ||
        !TestNtBorrowedName(TEST_PARAMETER_RESULT, L"sub\\nested\\missing.txt", &MissingName) ||
        !TestNtBorrowedName(TEST_PARAMETER_RESULT, L"sub\\.\\nested\\..\\nested\\file.txt", &DotName) ||
        !TestNtBorrowedName(TEST_PARAMETER_RESULT, L"sub/nested/file.txt", &SlashName))
    {
        return;
    }

    SavedCount = GetCurrentDirectoryW(0, NULL);
    TEST_OK(SavedCount != 0);
    if (SavedCount == 0)
    {
        return;
    }
    SavedDirectory = Mem_Alloc((SIZE_T)SavedCount * sizeof(WCHAR));
    TEST_OK(SavedDirectory != NULL);
    if (SavedDirectory == NULL)
    {
        return;
    }
    Count = GetCurrentDirectoryW(SavedCount, SavedDirectory);
    TEST_OK(Count != 0 && Count < SavedCount);
    if (Count == 0 || Count >= SavedCount)
    {
        goto Cleanup;
    }
    Count = GetTempPathW(ARRAYSIZE(TempPath), TempPath);
    TEST_OK(Count != 0 && Count < ARRAYSIZE(TempPath));
    if (Count == 0 || Count >= ARRAYSIZE(TempPath))
    {
        goto Cleanup;
    }
    Count = GetTempFileNameW(TempPath, L"MLE", 0, FixturePath);
    TEST_OK(Count != 0);
    if (Count == 0)
    {
        goto Cleanup;
    }
    TempFileCreated = TRUE;
    Status = NT_Win32PathToNtPath(FixturePath, &FixtureNtName);
    TEST_OK(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status) || !TestNtBorrowedName(TEST_PARAMETER_RESULT, FixtureNtName.Buffer, &AbsoluteName))
    {
        goto Cleanup;
    }
    Restored = DeleteFileW(FixturePath);
    TEST_OK(Restored);
    if (!Restored)
    {
        goto Cleanup;
    }
    TempFileCreated = FALSE;
    FixtureCreated = CreateDirectoryW(FixturePath, NULL) != FALSE;
    TEST_OK(FixtureCreated);
    if (!FixtureCreated)
    {
        goto Cleanup;
    }
    Status = IO_CreateWin32File(&Root,
                                FixturePath,
                                DirectoryAccess,
                                Share,
                                FILE_OPEN,
                                FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    TEST_OK(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
    {
        Root = NULL;
        goto Cleanup;
    }
    Status = IO_CreateFile(&File,
                           &EmptyName,
                           Root,
                           FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                           Share,
                           FILE_OPEN,
                           FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    TEST_OK(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
    {
        File = NULL;
        goto Cleanup;
    }
    TEST_OK(NT_SUCCESS(NtClose(File)));
    File = NULL;
    NT_InitObject(&DotObject, &EmptyName, OBJ_CASE_INSENSITIVE, Root);
    NativeStatus = NtQueryFullAttributesFile(&DotObject, &NativeAttributes);
    Status = IO_GetFileAttributes(&EmptyName, Root, &Attributes);
    TEST_OK(NT_SUCCESS(NativeStatus) && Status == NativeStatus &&
            BooleanFlagOn(Attributes.FileAttributes, FILE_ATTRIBUTE_DIRECTORY) &&
            Attributes.FileAttributes == NativeAttributes.FileAttributes &&
            Attributes.EndOfFile.QuadPart == NativeAttributes.EndOfFile.QuadPart);
    NativeStatus = NtOpenFile(&NativeFile,
                              FILE_READ_DATA | SYNCHRONIZE,
                              &DotObject,
                              &IoStatusBlock,
                              Share,
                              FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    Status = IO_OpenFile(&File, &EmptyName, Root, FILE_READ_DATA | SYNCHRONIZE, Share);
    TEST_OK(Status == NativeStatus && Status == STATUS_FILE_IS_A_DIRECTORY);
    if (NT_SUCCESS(NativeStatus))
    {
        NtClose(NativeFile);
    }
    if (NT_SUCCESS(Status))
    {
        NtClose(File);
    }
    File = NULL;
    for (i = 0; i < ARRAYSIZE(NativeFiles); i++)
    {
        if (!TestNtRootFile(TEST_PARAMETER_RESULT, Root, NativeFiles[i], FALSE))
        {
            goto Cleanup;
        }
    }
    VolumeKnown = GetVolumeInformationByHandleW(Root, NULL, 0, NULL, &MaximumComponentLength,
                                               &FileSystemFlags, NULL, 0);
    if (!VolumeKnown)
    {
        TEST_SKIP("Unable to query fixture filesystem flags: %lu", GetLastError());
    } else if (!BooleanFlagOn(FileSystemFlags, FILE_NAMED_STREAMS))
    {
        TEST_SKIP("The fixture filesystem does not support named streams.");
    } else
    {
        Status = IO_CreateFile(&File,
                               &BaseName,
                               Root,
                               FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                               Share,
                               FILE_CREATE,
                               FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
        TEST_OK(NT_SUCCESS(Status));
        if (!NT_SUCCESS(Status))
        {
            File = NULL;
            goto Cleanup;
        }
        BaseCreated = TRUE;
        TEST_OK(NT_SUCCESS(NtClose(File)));
        File = NULL;
        if (!TestNtRootFile(TEST_PARAMETER_RESULT, Root, L"C:file", TRUE))
        {
            goto Cleanup;
        }
        Status = IO_GetFileAttributes(&BaseName, Root, &Attributes);
        TEST_OK(NT_SUCCESS(Status) && Attributes.EndOfFile.QuadPart == 0);
        Status = IO_DeleteFile(&BaseName, Root);
        TEST_OK(NT_SUCCESS(Status));
        if (NT_SUCCESS(Status))
        {
            BaseCreated = FALSE;
        }
    }
    if (VolumeKnown && MaximumComponentLength >= 250)
    {
        if (!TestNtWin32LongFile(TEST_PARAMETER_RESULT, Root, FixturePath))
        {
            goto Cleanup;
        }
    } else
    {
        TEST_SKIP("The fixture filesystem cannot use the long-path test leaf.");
    }
    Status = IO_CreateFile(&Sub,
                           &SubName,
                           Root,
                           DirectoryAccess,
                           Share,
                           FILE_CREATE,
                           FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    TEST_OK(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
    {
        Sub = NULL;
        goto Cleanup;
    }
    Status = IO_CreateFile(&Nested,
                           &NestedName,
                           Root,
                           DirectoryAccess,
                           Share,
                           FILE_CREATE,
                           FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    TEST_OK(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
    {
        Nested = NULL;
        goto Cleanup;
    }
    Status = IO_CreateFile(&File,
                           &FileName,
                           Root,
                           FILE_READ_DATA | FILE_WRITE_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                           Share,
                           FILE_CREATE,
                           FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    TEST_OK(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
    {
        File = NULL;
        goto Cleanup;
    }
    FileCreated = TRUE;
    Status = IO_WriteFile(File, NULL, (PVOID)FileData, sizeof(FileData) - 1, &Bytes);
    TEST_OK(NT_SUCCESS(Status) && Bytes == sizeof(FileData) - 1);
    TEST_OK(NT_SUCCESS(NtClose(File)));
    File = NULL;
    Status = IO_OpenFile(&File,
                         &FileName,
                         Root,
                         FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                         Share);
    TEST_OK(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
    {
        File = NULL;
        goto Cleanup;
    }
    Status = IO_ReadFile(File, NULL, ReadData, sizeof(ReadData), &Bytes);
    TEST_OK(NT_SUCCESS(Status) && Bytes == sizeof(ReadData) && RtlEqualMemory(ReadData, FileData, sizeof(ReadData)));
    TEST_OK(NT_SUCCESS(NtClose(File)));
    File = NULL;
    NT_InitObject(&DotObject, &DotName, OBJ_CASE_INSENSITIVE, Root);
    NativeStatus = NtOpenFile(&NativeFile,
                              FILE_READ_DATA | SYNCHRONIZE,
                              &DotObject,
                              &IoStatusBlock,
                              Share,
                              FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    Status = IO_OpenFile(&File,
                         &DotName,
                         Root,
                         FILE_READ_DATA | SYNCHRONIZE,
                         Share);
    TEST_OK(Status == NativeStatus);
    if (NT_SUCCESS(NativeStatus))
    {
        NtClose(NativeFile);
    }
    if (NT_SUCCESS(Status))
    {
        NtClose(File);
    }
    File = NULL;
    NT_InitObject(&DotObject, &SlashName, OBJ_CASE_INSENSITIVE, Root);
    NativeStatus = NtOpenFile(&NativeFile,
                              FILE_READ_DATA | SYNCHRONIZE,
                              &DotObject,
                              &IoStatusBlock,
                              Share,
                              FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    Status = IO_OpenFile(&File, &SlashName, Root, FILE_READ_DATA | SYNCHRONIZE, Share);
    TEST_OK(Status == NativeStatus);
    if (NT_SUCCESS(NativeStatus))
    {
        NtClose(NativeFile);
    }
    if (NT_SUCCESS(Status))
    {
        NtClose(File);
    }
    File = NULL;
    Status = IO_GetFileAttributes(&FileName, Root, &Attributes);
    TEST_OK(NT_SUCCESS(Status) && Attributes.EndOfFile.QuadPart == sizeof(FileData) - 1 &&
            !BooleanFlagOn(Attributes.FileAttributes, FILE_ATTRIBUTE_DIRECTORY));
    Status = IO_GetFileAttributes(&NestedName, Root, &Attributes);
    TEST_OK(NT_SUCCESS(Status) && BooleanFlagOn(Attributes.FileAttributes, FILE_ATTRIBUTE_DIRECTORY));
    Status = IO_OpenFile(&File,
                         &MissingName,
                         Root,
                         FILE_READ_DATA | SYNCHRONIZE,
                         Share);
    TEST_OK(Status == STATUS_OBJECT_NAME_NOT_FOUND);
    if (NT_SUCCESS(Status))
    {
        NtClose(File);
    }
    File = NULL;
    NT_InitObject(&DotObject, &AbsoluteName, OBJ_CASE_INSENSITIVE, Root);
    NativeStatus = NtOpenFile(&NativeFile,
                              FILE_READ_DATA | SYNCHRONIZE,
                              &DotObject,
                              &IoStatusBlock,
                              Share,
                              FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    Status = IO_OpenFile(&File, &AbsoluteName, Root, FILE_READ_DATA | SYNCHRONIZE, Share);
    TEST_OK(Status == NativeStatus && !NT_SUCCESS(Status));
    if (NT_SUCCESS(NativeStatus))
    {
        NtClose(NativeFile);
    }
    if (NT_SUCCESS(Status))
    {
        NtClose(File);
    }
    File = NULL;

    DirectoryChanged = SetCurrentDirectoryW(TempPath) != FALSE;
    TEST_OK(DirectoryChanged);
    if (DirectoryChanged)
    {
        Status = IO_OpenFile(&File,
                             &FileName,
                             Root,
                             FILE_READ_DATA | SYNCHRONIZE,
                             Share);
        TEST_OK(NT_SUCCESS(Status));
        if (NT_SUCCESS(Status))
        {
            Status = IO_ReadFile(File, NULL, ReadData, sizeof(ReadData), &Bytes);
            TEST_OK(NT_SUCCESS(Status) && Bytes == sizeof(ReadData) &&
                    RtlEqualMemory(ReadData, FileData, sizeof(ReadData)));
            TEST_OK(NT_SUCCESS(NtClose(File)));
        }
        File = NULL;
    }
    Status = IO_DeleteFile(&FileName, Root);
    TEST_OK(NT_SUCCESS(Status));
    if (NT_SUCCESS(Status))
    {
        FileCreated = FALSE;
    }
    Status = IO_GetFileAttributes(&FileName, Root, &Attributes);
    TEST_OK(Status == STATUS_OBJECT_NAME_NOT_FOUND);
Cleanup:
    if (DirectoryChanged)
    {
        Restored = SetCurrentDirectoryW(SavedDirectory);
        TEST_OK(Restored);
    }
    if (File != NULL)
    {
        NtClose(File);
    }
    if (FileCreated)
    {
        TEST_OK(NT_SUCCESS(IO_DeleteFile(&FileName, Root)));
    }
    if (BaseCreated)
    {
        TEST_OK(NT_SUCCESS(IO_DeleteFile(&BaseName, Root)));
    }
    if (Nested != NULL)
    {
        TEST_OK(NT_SUCCESS(IO_DisposeFile(Nested)));
        TEST_OK(NT_SUCCESS(NtClose(Nested)));
    }
    if (Sub != NULL)
    {
        TEST_OK(NT_SUCCESS(IO_DisposeFile(Sub)));
        TEST_OK(NT_SUCCESS(NtClose(Sub)));
    }
    if (Root != NULL)
    {
        TEST_OK(NT_SUCCESS(IO_DisposeFile(Root)));
        TEST_OK(NT_SUCCESS(NtClose(Root)));
    } else if (FixtureCreated)
    {
        TEST_OK(RemoveDirectoryW(FixturePath));
    }
    if (TempFileCreated)
    {
        TEST_OK(DeleteFileW(FixturePath));
    }
    if (FixtureNtName.Buffer != NULL)
    {
        TEST_OK(NT_FreeNtPath(&FixtureNtName));
    }
    Mem_Free(SavedDirectory);
}
