/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Windows Platform

Author:
    neoxed (neoxed@gmail.com) Jul 17, 2005

Abstract:
    This module implements file I/O wrappers for Windows.

--*/

#include "alcoholicz.h"

/*++

FileOpen

    Opens a file.

Arguments:
    path    - Path of the file to be opened.

    access  - Access desired to the file.
              FACCESS_READ       - Open for reading only
              FACCESS_WRITE      - Open for writing only
              FACCESS_READWRITE  - Open for reading and writing

    exists  - Action to take on files that exist and do not exist.
              FEXIST_ALWAYS_NEW  - Always create a new file.
              FEXIST_NEW         - Create a new file; fails if it does exist.
              FEXIST_PRESENT     - Open a present file; fails if it does not exist.
              FEXIST_REGARDLESS  - Open a file regardless of its existence.
              FEXIST_TRUNCATE    - Truncate a file; fails if it does not exist.

    options - File attributes and flags.
              FOPTION_HIDDEN     - Set the file's attribute as hidden.
              FOPTION_RANDOM     - Optimize system caching for random access.
              FOPTION_SEQUENTIAL - Optimize system caching for sequential access.

Return Value:
    If the function succeeds, the return value is a handle to a specified file.
    If the function fails, the return value is INVALID_HANDLE_VALUE.

--*/
HANDLE
FileOpen(
    const tchar_t *path,
    uint32_t access,
    uint32_t exists,
    uint32_t options
    )
{
    DWORD share = 0;

    ASSERT(path   != NULL);
    ASSERT(access != 0);

    if (access == FACCESS_READ) {
        share = FILE_SHARE_READ;
    }
    return CreateFile(path, access, share, NULL, exists, options, NULL);
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
    HANDLE handle,
    uint64_t *size
    )
{
    DWORD highSize;
    DWORD lowSize;

    ASSERT(handle != INVALID_HANDLE_VALUE);
    ASSERT(size   != NULL);

    lowSize = GetFileSize(handle, &highSize);
    if (lowSize == INVALID_FILE_SIZE) {
        *size = 0;
        return FALSE;
    }

    *size = (((uint64_t)highSize) << 32) + lowSize;
    return TRUE;
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
    HANDLE handle,
    int64_t offset,
    int method
    )
{
    LARGE_INTEGER largeInt;

    ASSERT(handle != INVALID_HANDLE_VALUE);
    ASSERT(method == FILE_BEGIN || method == FILE_CURRENT || method == FILE_END);

    largeInt.QuadPart = offset;
    largeInt.LowPart = SetFilePointer(handle,
        largeInt.LowPart, &largeInt.HighPart, (DWORD)method);

   if (largeInt.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
      return -1;
   }

   return largeInt.QuadPart;
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
    HANDLE handle,
    void *buffer,
    size_t bytesToRead,
    size_t *bytesRead
    )
{
    BOOL result;
    DWORD amount;

    ASSERT(handle != INVALID_HANDLE_VALUE);
    ASSERT(buffer != NULL);

    result = ReadFile(handle, buffer, (DWORD)bytesToRead, &amount, NULL);

    if (bytesRead != NULL) {
        *bytesRead = (size_t)amount;
    }

    return (bool_t)result;
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
    HANDLE handle,
    const void *buffer,
    size_t bytesToWrite,
    size_t *bytesWritten
    )
{
    BOOL result;
    DWORD amount;

    ASSERT(handle != INVALID_HANDLE_VALUE);
    ASSERT(buffer != NULL);

    result = WriteFile(handle, buffer, (DWORD)bytesToWrite, &amount, NULL);

    if (bytesWritten != NULL) {
        *bytesWritten = (size_t)amount;
    }

    return (bool_t)result;
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
    HANDLE handle
    )
{
    ASSERT(handle != INVALID_HANDLE_VALUE);
    return (bool_t)CloseHandle(handle);
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

    return (bool_t)CopyFile(sourcePath, destPath, !replace);
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
    DWORD flags;

    ASSERT(sourcePath != NULL);
    ASSERT(destPath   != NULL);

    flags = replace ? MOVEFILE_REPLACE_EXISTING : 0;
    return (bool_t)MoveFileEx(sourcePath, destPath, flags|MOVEFILE_COPY_ALLOWED);
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
    return (bool_t)DeleteFile(path);
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
    HANDLE handle;

    ASSERT(path != NULL);

    //
    // Faster than using GetFileAttributes() because the file
    // is not actually opened (for reading data and/or attributes).
    //
    handle = CreateFile(path, 0, FILE_SHARE_DELETE|FILE_SHARE_READ|
        FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if (handle != INVALID_HANDLE_VALUE) {
        CloseHandle(handle);
        return TRUE;
    }

    return FALSE;
}

/*++

FileIsLink

    Tests for the existence of a NTFS junction.

Arguments:
    path    - Path to be tested.

Return Value:
    If the path is a junction, the return value is non-zero. If the path is
    not a junction, the return value is zero.

--*/
bool_t
FileIsLink(
    const tchar_t *path
    )
{
    DWORD attribs;

    ASSERT(path != NULL);

    attribs = GetFileAttributes(path);
    if (attribs != INVALID_FILE_ATTRIBUTES &&
            attribs & FILE_ATTRIBUTE_REPARSE_POINT) {
        return TRUE;
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
    DWORD attribs;

    ASSERT(path != NULL);

    attribs = GetFileAttributes(path);
    if (attribs != INVALID_FILE_ATTRIBUTES &&
            !(attribs & FILE_ATTRIBUTE_DIRECTORY) &&
            !(attribs & FILE_ATTRIBUTE_REPARSE_POINT)) {
        return TRUE;
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
    return (bool_t)CreateDirectory(path, NULL);
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
    return (bool_t)RemoveDirectory(path);
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
    DWORD attribs;

    ASSERT(path != NULL);

    attribs = GetFileAttributes(path);
    if (attribs != INVALID_FILE_ATTRIBUTES &&
            attribs & FILE_ATTRIBUTE_DIRECTORY) {
        return TRUE;
    }

    return FALSE;
}
