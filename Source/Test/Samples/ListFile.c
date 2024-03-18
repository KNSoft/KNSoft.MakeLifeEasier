#include "../Test.h"

static PCSTR g_pszDescription = "This sample lists all files in current working directory.";

BOOL Sample_ListFile()
{
    NTSTATUS Status;
    HANDLE DirectoryHandle;
    FILE_FIND FindData;
    PCURDIR CurDir;
    PFILE_FULL_DIR_INFORMATION pData;
    UNICODE_STRING NtName;

    PrintTitle(__FUNCTION__, g_pszDescription);

    CurDir = &NtCurrentPeb()->ProcessParameters->CurrentDirectory;
    PrintF("Current directory: %wZ\n", &CurDir->DosPath);

    /* Initialize the enumeration */
    Status = RtlDosPathNameToNtPathName_U_WithStatus(CurDir->DosPath.Buffer, &NtName, NULL, NULL);
    if (!NT_SUCCESS(Status))
    {
        PrintF("RtlDosPathNameToNtPathName_U_WithStatus failed with 0x%08lX\n", Status);
        return FALSE;
    }
    Status = File_OpenDirectory(&DirectoryHandle,
                                &NtName,
                                FILE_LIST_DIRECTORY | SYNCHRONIZE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE);
    if (NtName.Buffer != NULL)
    {
        RtlFreeHeap(NtGetProcessHeap(), 0, NtName.Buffer);
    }
    if (!NT_SUCCESS(Status))
    {
        PrintF("File_OpenDirectory failed with 0x%08lX\n", Status);
        return FALSE;
    }

    Status = File_BeginFind(&FindData, DirectoryHandle, NULL, FileFullDirectoryInformation);
    if (!NT_SUCCESS(Status))
    {
        NtClose(DirectoryHandle);
        PrintF("File_BeginFind failed with 0x%08lX\n", Status);
        return FALSE;
    }

    /* Enumerate files */
    while (NT_SUCCESS(Status) && FindData.HasData)
    {
        /* Read data from the return buffer */
        pData = (PFILE_FULL_DIR_INFORMATION)FindData.Buffer;
        while (TRUE)
        {
            /* Print file name */
            NtName.Buffer = pData->FileName;
            NtName.MaximumLength = NtName.Length = (USHORT)pData->FileNameLength;
            PrintF("\t%wZ\n", &NtName);

            /* Go to the next entry */
            if (!pData->NextEntryOffset)
            {
                break;
            }
            pData = (PFILE_FULL_DIR_INFORMATION)Add2Ptr(pData, pData->NextEntryOffset);
        };

        Status = File_ContinueFind(&FindData);
    }
    File_EndFind(&FindData);
    NtClose(DirectoryHandle);

    if (NT_SUCCESS(Status))
    {
        return TRUE;
    } else
    {
        PrintF("File_Find failed with 0x%08lX\n", Status);
        return FALSE;
    }
}
