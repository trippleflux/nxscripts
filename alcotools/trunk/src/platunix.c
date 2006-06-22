/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Unix Platform

Author:
    neoxed (neoxed@gmail.com) Jul 17, 2005

Abstract:
    This module implements file I/O wrappers for BSD/Linux/UNIX.

--*/

#include "alcoholicz.h"

/*++

FileOpen

    Opens a file.

Arguments:
    path    - Specifies the path of the file to be opened.

    access  - Access desired to the file.
              FACCESS_READ       - Open for reading only
              FACCESS_WRITE      - Open for writing only
              FACCESS_READWRITE  - Open for reading and writing

    exists  - Action to take on files that exist and do not exist.
              FEXIST_ALWAYS_NEW  - Always create a new file.
              FEXIST_NEW         - Create a new file; fails if it does exist.
              FEXIST_PRESENT     - Open a present file; fails if it does NOT exist.
              FEXIST_REGARDLESS  - Open a file regardless of its existence.
              FEXIST_TRUNCATE    - Truncate a file; fails if it does NOT exist.

    options - File attributes and flags.
              FOPTION_HIDDEN     - Set the file's attribute as hidden.
              FOPTION_RANDOM     - Optimize system caching for random access.
              FOPTION_SEQUENTIAL - Optimize system caching for sequential access.

Return Value:
    If the function succeeds, the return value is a handle to a specified file.
    If the function fails, the return value is INVALID_HANDLE_VALUE.

--*/
int
FileOpen(
    const tchar_t *path,
    uint32_t access,
    uint32_t exists,
    uint32_t options
    )
{
    ASSERT(path   != NULL);
    ASSERT(access != 0);

    return t_open(path, (int)(access|exists|options));
}

/*++

FileSize

    Retrieves the size of a file.

Arguments:
    handle  - Handle to an open file.

    size    - Pointer to the variable that receives the file's size, in bytes.

Return Value:
    If the function succeeds, the return value is non-zero. If the
    function fails, the return value is zero.

--*/
bool_t
FileSize(
    int handle,
    uint64_t *size
    )
{
    struct stat statBuf;

    ASSERT(handle != -1);
    ASSERT(size   != NULL);

    if (fstat(handle, &statBuf) == 0) {
        *size = (uint64_t)statBuf.st_size;
        return TRUE;
    }

    *size = 0;
    return FALSE;
}

/*++

FileSeek

    Moves the file's current position.

Arguments:
    handle  - Handle to an open file.

    offset  - Number of bytes from the current position.

    method  - Seek method, must be: FILE_BEGIN, FILE_CURRENT, or FILE_END.

Return Value:
    If the function succeeds, the return value is the file's new position. If
    the function fails, the return value is -1.

--*/
int64_t
FileSeek(
    int handle,
    int64_t offset,
    int method
    )
{
    ASSERT(handle != -1);
    ASSERT(method == FILE_BEGIN || method == FILE_CURRENT || method == FILE_END);

    return (int64_t)lseek(handle, (off_t)offset, method);
}

/*++

FileRead

    Reads data from a file's current position.

Arguments:
    handle      - Handle to an open file.

    buffer      - Pointer to the buffer that receives the data read from a file.

    bytesToRead - Number of bytes to read from the file.

    bytesRead   - Pointer to a variable that receives the number of bytes
                  read. This argument can be NULL.

Return Value:
    If the function succeeds, the return value is non-zero. If the
    function fails, the return value is zero.

--*/
bool_t
FileRead(
    int handle,
    void *buffer,
    size_t bytesToRead,
    size_t *bytesRead
    )
{
    ssize_t amount;

    ASSERT(handle != -1);
    ASSERT(buffer != NULL);

    amount = read(handle, buffer, bytesToRead);

    if (bytesRead != NULL) {
        *bytesRead = (size_t)amount;
    }

    return (amount != -1) ? TRUE : FALSE;
}

/*++

FileWrite

    Writes data to a file's current position.

Arguments:
    handle       - Handle to an open file.

    buffer       - Pointer to a buffer that contains the data to be written.

    bytesToWrite - Number of bytes to be written.

    bytesWritten - Pointer to a variable that receives the number of bytes
                   written. This argument can be NULL.

Return Value:
    If the function succeeds, the return value is non-zero. If the
    function fails, the return value is zero.

--*/
bool_t
FileWrite(
    int handle,
    const void *buffer,
    size_t bytesToWrite,
    size_t *bytesWritten
    )
{
    ssize_t amount;

    ASSERT(handle != -1);
    ASSERT(buffer != NULL);

    amount = write(handle, buffer, bytesToWrite);

    if (bytesWritten != NULL) {
        *bytesWritten = (size_t)amount;
    }

    return (amount != -1) ? TRUE : FALSE;
}

/*++

FileClose

    Closes an open file.

Arguments:
    handle  - Handle to an open file.

Return Value:
    If the function succeeds, the return value is non-zero. If the
    function fails, the return value is zero.

--*/
bool_t
FileClose(
    int handle
    )
{
    ASSERT(handle != -1);
    return (close(handle) == 0) ? TRUE : FALSE;
}


/*++

FileCopy

    Copies an existing file to a new file.

Arguments:
    sourcePath - Source file path.

    destPath   - Destination file path.

    replace    - Replace the destination file if it exists.

Return Value:
    If the function succeeds, the return value is non-zero. If the
    function fails, the return value is zero.

--*/
bool_t
FileCopy(
    const tchar_t *sourcePath,
    const tchar_t *destPath,
    bool_t replace
    )
{
    ASSERT(sourcePath != NULL);
    ASSERT(destPath   != NULL);

    // TODO: borrow from mv.c
    return FALSE;
}

/*++

FileMove

    Moves an existing file to a new location.

Arguments:
    sourcePath - Source file path.

    destPath   - Destination file path.

    replace    - Replace the destination file if it exists.

Return Value:
    If the function succeeds, the return value is non-zero. If the
    function fails, the return value is zero.

--*/
bool_t
FileMove(
    const tchar_t *sourcePath,
    const tchar_t *destPath,
    bool_t replace
    )
{
    ASSERT(sourcePath != NULL);
    ASSERT(destPath   != NULL);

    // TODO: borrow from mv.c
    return FALSE;
}

/*++

FileRemove

    Removes an existing file.

Arguments:
    path    - File to be removed.

Return Value:
    If the function succeeds, the return value is non-zero. If the
    function fails, the return value is zero.

--*/
bool_t
FileRemove(
    const tchar_t *path
    )
{
    ASSERT(path != NULL);
    return (unlink(path) == 0) ? TRUE : FALSE;
}

/*++

FileExists

    Tests for the existence of a path.

Arguments:
    path    - Path to be tested.

Return Value:
    If the path exists, the return value is non-zero. If the path does not
    exist, the return value is zero.

--*/
bool_t
FileExists(
    const tchar_t *path
    )
{
    ASSERT(path != NULL);
    return (access(path, F_OK) == 0) ? TRUE : FALSE;
}

/*++

FileIsLink

    Tests for the existence of a symbolic link.

Arguments:
    path    - Path to be tested.

Return Value:
    If the path is a symbolic link, the return value is non-zero. If the path is
    not a symbolic link, the return value is zero.

--*/
bool_t
FileIsLink(
    const tchar_t *path
    )
{
    struct stat statBuf;

    ASSERT(path != NULL);

    if (lstat(path, &statbuf) == 0) {
        return (bool_t)S_ISLNK(statBuf.st_mode);
    }

    return FALSE;
}

/*++

FileIsRegular

    Tests for the existence of a regular file.

Arguments:
    path    - Path to be tested.

Return Value:
    If the path is a regular file, the return value is non-zero. If the path is
    not a regular file, the return value is zero.

--*/
bool_t
FileIsRegular(
    const tchar_t *path
    )
{
    struct stat statBuf;

    ASSERT(path != NULL);

    if (lstat(path, &statbuf) == 0) {
        return (bool_t)S_ISREG(statBuf.st_mode);
    }

    return FALSE;
}


/*++

DirCreate

    Creates a new directory.

Arguments:
    path    - Directory to be created.

Return Value:
    If the function succeeds, the return value is non-zero. If the
    function fails, the return value is zero.

--*/
bool_t
DirCreate(
    const tchar_t *path
    )
{
    ASSERT(path != NULL);
    return (mkdir(path) == 0) ? TRUE : FALSE;
}

/*++

DirRemove

    Removes an existing empty directory.

Arguments:
    path    - Directory to be removed.

Return Value:
    If the function succeeds, the return value is non-zero. If the
    function fails, the return value is zero.

--*/
bool_t
DirRemove(
    const tchar_t *path
    )
{
    ASSERT(path != NULL);
    return (rmdir(path) == 0) ? TRUE : FALSE;
}

/*++

DirExists

    Tests for the existence of a directory.

Arguments:
    path    - Path to be tested.

Return Value:
    If the path is a directory, the return value is non-zero. If the path is
    not a directory, the return value is zero.

--*/
bool_t
DirExists(
    const tchar_t *path
    )
{
    struct stat statBuf;

    ASSERT(path != NULL);

    if (lstat(path, &statbuf) == 0) {
        return (bool_t)S_ISDIR(statBuf.st_mode);
    }

    return FALSE;
}


DirGetCWD / DirSetCWD
DirGetCurrent / DirSetCurrent
