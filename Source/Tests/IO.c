#include "Test.h"

TEST_FUNC(IO_Pipe)
{
    UNICODE_STRING DirectoryPath = RTL_CONSTANT_STRING(DEVICE_NAMED_PIPE);
    HANDLE Directory, Handle, Peer;
    NTSTATUS Status;
    ULONG Mode;

    Status = IO_CreateFile(&Directory,
                           &DirectoryPath,
                           NULL,
                           SYNCHRONIZE | GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN,
                           FILE_SYNCHRONOUS_IO_NONALERT);
    TEST_OK(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
    {
        return;
    }
    for (Mode = FILE_PIPE_INBOUND; Mode <= FILE_PIPE_FULL_DUPLEX; Mode++)
    {
        Status = IO_CreatePipe(Directory, &Handle, &Peer, Mode, 4096);
        TEST_OK(NT_SUCCESS(Status));
        if (!NT_SUCCESS(Status))
        {
            break;
        }
        NtClose(Peer);
        NtClose(Handle);
    }
    TEST_OK(IO_CreatePipe(Directory,
                          &Handle,
                          &Peer,
                          FILE_PIPE_FULL_DUPLEX + 1,
                          4096) == STATUS_INVALID_PARAMETER);
    NtClose(Directory);
}
