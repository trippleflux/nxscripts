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

Io_VfsFlush

    Flushes the directory cache for a specified path.

Arguments:
    memory  - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc. The
              buffer size must be at least "strlen(path) + 1".

    path    - Pointer to a null-terminated string that specifies the physical
              directory path to be flushed (e.g. "C:\foo\bar").

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

--*/
BOOL
STDCALL
Io_VfsFlush(
    IO_MEMORY *memory,
    const char *path
    )
{
    size_t pathLength;

    // Validate arguments.
    if (memory == NULL || path == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    StringCchLengthA(path, STRSAFE_MAX_CCH, &pathLength);
    pathLength++; // Include terminating null.

    // Check if the shared memory block is large enough.
    if (memory->size < pathLength) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    CopyMemory(memory->block, path, pathLength);

    // ioFTPD appears to return 1 on both success and failure.
    if (Io_ShmQuery(memory, DC_DIRECTORY_MARKDIRTY, 5000) == 1) {
        return TRUE;
    }

    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}

/*++

Io_VfsRead

    Retrieves the ownership and permissions for a file or directory.

Arguments:
    memory  - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc. The
              buffer size must be at least "sizeof(DC_VFS) + strlen(path) + 1".

    path    - Pointer to a null-terminated string that specifies the physical
              path to read VFS information from (e.g. "C:\foo\bar").

    vfs     - Pointer to an IO_VFS structure that receives the ownership and
              permission information.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

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
    size_t pathLength;

    // Validate arguments.
    if (memory == NULL || path == NULL || vfs == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    StringCchLengthA(path, STRSAFE_MAX_CCH, &pathLength);
    pathLength++; // Include terminating null.

    // Check if the shared memory block is large enough.
    if (memory->size < sizeof(DC_VFS) + pathLength) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Initialise the DC_VFS structure.
    dcVfs = (DC_VFS *)memory->block;
    dcVfs->dwBuffer = (DWORD)pathLength;
    CopyMemory(dcVfs->pBuffer, path, dcVfs->dwBuffer);

    if (!Io_ShmQuery(memory, DC_FILEINFO_READ, 5000)) {
        vfs->userId   = dcVfs->Uid;
        vfs->groupId  = dcVfs->Gid;
        vfs->fileMode = dcVfs->dwFileMode;
        return TRUE;
    }

    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}

/*++

Io_VfsWrite

    Sets the ownership and permissions for a file or directory.

Arguments:
    memory  - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc. The
              buffer size must be at least "sizeof(DC_VFS) + strlen(path) + 1".

    path    - Pointer to a null-terminated string that specifies the physical
              path to write VFS information to (e.g. "C:\foo\bar").

    vfs     - Pointer to an IO_VFS structure that contains the new ownership
              and permission information.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

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
    size_t pathLength;

    // Validate arguments.
    if (memory == NULL || path == NULL || vfs == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    StringCchLengthA(path, STRSAFE_MAX_CCH, &pathLength);
    pathLength++; // Include terminating null.

    // Check if the shared memory block is large enough.
    if (memory->size < sizeof(DC_VFS) + pathLength) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Initialise the DC_VFS structure.
    dcVfs = (DC_VFS *)memory->block;
    dcVfs->Uid        = vfs->userId;
    dcVfs->Gid        = vfs->groupId;
    dcVfs->dwFileMode = vfs->fileMode;
    dcVfs->dwBuffer   = (DWORD)pathLength;
    CopyMemory(dcVfs->pBuffer, path, dcVfs->dwBuffer);

    if (!Io_ShmQuery(memory, DC_FILEINFO_WRITE, 5000)) {
        return TRUE;
    }

    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}
