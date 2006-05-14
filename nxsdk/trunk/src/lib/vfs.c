/*

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    Virtual File System

Author:
    neoxed (neoxed@gmail.com) May 13, 2006

Abstract:
    Virtual file system functions.

*/

#include "lib.h"

/*++

Io_VfsRead

    Retrieves the ownership and permissions for a file or directory.

Arguments:
    session - Pointer to an initialised IO_SESSION structure.

    memory  - Pointer to an allocated IO_MEMORY structure. The buffer
              size must be at least sizeof(DC_VFS) + strlen(path) + 1.

    path    - The file or directory path to query.

    vfs     - Pointer to a IO_VFS structure.

Return Value:
    A standard boolean result.

--*/
BOOL
Io_VfsRead(
    IO_SESSION *session,
    IO_MEMORY *memory,
    const char *path,
    IO_VFS *vfs
    )
{
    DC_VFS *dcVfs;

    assert(session != NULL);
    assert(memory  != NULL);
    assert(path    != NULL);
    assert(vfs != NULL);
    assert(memory->bytes >= sizeof(DC_VFS) + strlen(path) + 1);
    DebugPrint("Io_VfsRead: path=%s vfs=%p\n", path, vfs);

    // Initialise the DC_VFS structure.
    dcVfs = (DC_VFS *)memory->block;
    dcVfs->dwBuffer = strlen(path) + 1;
    CopyMemory(dcVfs->pBuffer, path, dcVfs->dwBuffer);

    if (!Io_ShmQuery(session, memory, DC_FILEINFO_READ, 5000)) {
        vfs->userId   = dcVfs->Uid;
        vfs->groupId  = dcVfs->Gid;
        vfs->fileMode = dcVfs->dwFileMode;

        DebugPrint("Io_VfsRead: OKAY\n");
        return TRUE;
    }

    DebugPrint("Io_VfsRead: FAIL\n");
    return FALSE;
}

/*++

Io_VfsWrite

    Sets the ownership and permissions for a file or directory.

Arguments:
    session - Pointer to an initialised IO_SESSION structure.

    memory  - Pointer to an allocated IO_MEMORY structure. The buffer
              size must be at least sizeof(DC_VFS) + strlen(path) + 1.

    path    - The file or directory path to query.

    vfs     - Pointer to a IO_VFS structure.

Return Value:
    A standard boolean result.

--*/
BOOL
Io_VfsWrite(
    IO_SESSION *session,
    IO_MEMORY *memory,
    const char *path,
    const IO_VFS *vfs
    )
{
    DC_VFS *dcVfs;

    assert(session != NULL);
    assert(memory  != NULL);
    assert(path    != NULL);
    assert(vfs != NULL);
    assert(memory->bytes >= sizeof(DC_VFS) + strlen(path) + 1);
    DebugPrint("Io_VfsWrite: path=%s vfs=%p\n", path, vfs);

    // Initialise the DC_VFS structure.
    dcVfs = (DC_VFS *)memory->block;
    dcVfs->Uid        = vfs->userId;
    dcVfs->Gid        = vfs->groupId;
    dcVfs->dwFileMode = vfs->fileMode;
    dcVfs->dwBuffer   = strlen(path) + 1;
    CopyMemory(dcVfs->pBuffer, path, dcVfs->dwBuffer);

    if (!Io_ShmQuery(session, memory, DC_FILEINFO_WRITE, 5000)) {
        DebugPrint("Io_VfsWrite: OKAY\n");
        return TRUE;
    }

    DebugPrint("Io_VfsWrite: FAIL\n");
    return FALSE;
}


/*++

Io_VfsFlush

    Flush the directory cache for a specified path.

Arguments:
    session - Pointer to an initialised IO_SESSION structure.

    memory  - Pointer to an allocated IO_MEMORY structure. The memory
              buffer must be at least strlen(dirPath) + 1

    dirPath - The directory path to flush.

Return Value:
    A standard boolean result.

--*/
BOOL
Io_VfsFlush(
    IO_SESSION *session,
    IO_MEMORY *memory,
    const char *dirPath
    )
{
    assert(session != NULL);
    assert(memory  != NULL);
    assert(dirPath != NULL);
    assert(memory->bytes >= strlen(dirPath) + 1);
    DebugPrint("Io_VfsFlush: dirPath=%s\n", dirPath);

    StringCchCopyA((char *)memory->block, (size_t)memory->bytes, dirPath);

    // ioFTPD appears to return 1 on both success and failure.
    if (Io_ShmQuery(session, memory, DC_DIRECTORY_MARKDIRTY, 5000) == 1) {
        DebugPrint("Io_VfsFlush: OKAY\n");
        return TRUE;
    }

    DebugPrint("Io_VfsFlush: FAIL\n");
    return FALSE;
}
