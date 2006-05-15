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
    memory  - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc. The
              buffer size must be at least "sizeof(DC_VFS) + strlen(path) + 1".

    path    - Pointer to a null-terminated string that specifies the path to
              read VFS information from.

    vfs     - Pointer to an IO_VFS structure that receives the ownership and
              permission information.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
BOOL
STDCALL
Io_VfsRead(
    IO_MEMORY *memory,
    const char *path,
    IO_VFS *vfs
    )
{
    DC_VFS *dcVfs;

    assert(memory  != NULL);
    assert(path    != NULL);
    assert(vfs != NULL);
    assert(memory->size >= sizeof(DC_VFS) + strlen(path) + 1);
    DebugPrint("Io_VfsRead: path=%s vfs=%p\n", path, vfs);

    // Initialise the DC_VFS structure.
    dcVfs = (DC_VFS *)memory->block;
    dcVfs->dwBuffer = strlen(path) + 1;
    CopyMemory(dcVfs->pBuffer, path, dcVfs->dwBuffer);

    if (!Io_ShmQuery(memory, DC_FILEINFO_READ, 5000)) {
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
    memory  - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc. The
              buffer size must be at least "sizeof(DC_VFS) + strlen(path) + 1".

    path    - Pointer to a null-terminated string that specifies the path to
              write VFS information to.

    vfs     - Pointer to an IO_VFS structure that contains the new ownership
              and permission information.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
BOOL
STDCALL
Io_VfsWrite(
    IO_MEMORY *memory,
    const char *path,
    const IO_VFS *vfs
    )
{
    DC_VFS *dcVfs;

    assert(memory  != NULL);
    assert(path    != NULL);
    assert(vfs != NULL);
    assert(memory->size >= sizeof(DC_VFS) + strlen(path) + 1);
    DebugPrint("Io_VfsWrite: path=%s vfs=%p\n", path, vfs);

    // Initialise the DC_VFS structure.
    dcVfs = (DC_VFS *)memory->block;
    dcVfs->Uid        = vfs->userId;
    dcVfs->Gid        = vfs->groupId;
    dcVfs->dwFileMode = vfs->fileMode;
    dcVfs->dwBuffer   = strlen(path) + 1;
    CopyMemory(dcVfs->pBuffer, path, dcVfs->dwBuffer);

    if (!Io_ShmQuery(memory, DC_FILEINFO_WRITE, 5000)) {
        DebugPrint("Io_VfsWrite: OKAY\n");
        return TRUE;
    }

    DebugPrint("Io_VfsWrite: FAIL\n");
    return FALSE;
}


/*++

Io_VfsFlush

    Flushes the directory cache for a specified path.

Arguments:
    memory  - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc. The
              buffer size must be at least "strlen(dirPath) + 1".

    dirPath - Pointer to a null-terminated string that specifies the directory
              path to be flushed.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
BOOL
STDCALL
Io_VfsFlush(
    IO_MEMORY *memory,
    const char *dirPath
    )
{
    assert(memory  != NULL);
    assert(dirPath != NULL);
    assert(memory->size >= strlen(dirPath) + 1);
    DebugPrint("Io_VfsFlush: dirPath=%s\n", dirPath);

    StringCchCopyA((char *)memory->block, (size_t)memory->size, dirPath);

    // ioFTPD appears to return 1 on both success and failure.
    if (Io_ShmQuery(memory, DC_DIRECTORY_MARKDIRTY, 5000) == 1) {
        DebugPrint("Io_VfsFlush: OKAY\n");
        return TRUE;
    }

    DebugPrint("Io_VfsFlush: FAIL\n");
    return FALSE;
}
