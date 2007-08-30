/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    Group File Backend

Author:
    neoxed (neoxed@gmail.com) Jun 5, 2006

Abstract:
    Group file storage backend.

*/

#include <base.h>
#include <backends.h>

static DWORD GroupRead(CHAR *filePath, GROUPFILE *groupFile)
{
    CHAR    *buffer = NULL;
    DWORD   bytesRead;
    DWORD   fileSize;
    DWORD   result;
    HANDLE  fileHandle;

    ASSERT(filePath != NULL);
    ASSERT(groupFile != NULL);
    TRACE("filePath=%s groupFile=%p\n", filePath, groupFile);

    // Open group file
    fileHandle = CreateFileA(filePath,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (fileHandle == INVALID_HANDLE_VALUE) {
        result = GetLastError();
        TRACE("Unable to open file \"%s\" (error %lu).\n", filePath, result);
        goto failed;
    }

    // Retrieve file size
    fileSize = GetFileSize(fileHandle, NULL);
    if (fileSize == INVALID_FILE_SIZE || fileSize < 5) {
        result = INVALID_FILE_SIZE;
        TRACE("Unable to retrieve size of file \"%s\" (error %lu).\n", filePath, result);
        goto failed;
    }

    // Allocate read buffer
    buffer = Io_Allocate(fileSize + 1);
    if (buffer == NULL) {
        result = ERROR_NOT_ENOUGH_MEMORY;
        TRACE("Unable to allocate read buffer.\n");
        goto failed;
    }

    // Read group file to buffer
    if (!ReadFile(fileHandle, buffer, fileSize, &bytesRead, NULL) || bytesRead < 5) {
        result = GetLastError();
        TRACE("Unable to write file \"%s\" (error %lu).\n", filePath, result);
        goto failed;
    }

    // Pad buffer with a new-line
    buffer[bytesRead] = '\n';
    bytesRead++;

    // Parse buffer, also initializing the GROUPFILE structure
    Io_Ascii2GroupFile(buffer, bytesRead, groupFile);

    // Save file handle
    groupFile->lpInternal = fileHandle;

    Io_Free(buffer);
    return ERROR_SUCCESS;

failed:
    if (buffer != NULL) {
        Io_Free(buffer);
    }
    CloseHandle(fileHandle);

    return result;
}

DWORD FileGroupCreate(INT32 groupId, GROUPFILE *groupFile)
{
    CHAR  *defaultPath = NULL;
    CHAR  *targetPath = NULL;
    CHAR  buffer[12];
    DWORD result;

    ASSERT(groupId != -1);
    ASSERT(groupFile != NULL);
    TRACE("groupId=%d groupFile=%p\n", groupId, groupFile);

    // Retrieve default group location
    defaultPath = Io_ConfigGetPath("Locations", "Group_Files", "Default.Group", NULL);
    if (defaultPath == NULL) {
        TRACE("Unable to retrieve default file location.\n");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Retrieve group location
    StringCchPrintfA(buffer, ELEMENT_COUNT(buffer), "%i", groupId);
    targetPath = Io_ConfigGetPath("Locations", "Group_Files", buffer, NULL);
    if (targetPath == NULL) {
        TRACE("Unable to retrieve file location.\n");
        result = ERROR_NOT_ENOUGH_MEMORY;
    } else {

        // Copy default file to target file
        if (!CopyFileA(defaultPath, targetPath, FALSE)) {
            result = GetLastError();
            TRACE("Unable to copy default file (error %lu).\n", result);

        } else {
            // Read group file (copy of "Default.Group")
            result = GroupRead(targetPath, groupFile);
        }
    }

    if (defaultPath != NULL) {
        Io_Free(defaultPath);
    }
    if (targetPath != NULL) {
        Io_Free(targetPath);
    }
    return result;
}

DWORD FileGroupDelete(INT32 groupId)
{
    CHAR  buffer[12];
    CHAR  *filePath;
    DWORD result;

    ASSERT(groupId != -1);
    TRACE("groupId=%d\n", groupId);

    // Retrieve group file location
    StringCchPrintfA(buffer, ELEMENT_COUNT(buffer), "%i", groupId);
    filePath = Io_ConfigGetPath("Locations", "Group_Files", buffer, NULL);
    if (filePath == NULL) {
        TRACE("Unable to retrieve file location.\n");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Delete group file and free resources
    if (DeleteFileA(filePath)) {
        result = ERROR_SUCCESS;
    } else {
        result = GetLastError();
        TRACE("Unable to delete file \"%s\" (error %lu).\n", filePath, result);
    }

    Io_Free(filePath);
    return result;
}

DWORD FileGroupOpen(INT32 groupId, GROUPFILE *groupFile)
{
    CHAR    buffer[12];
    CHAR    *filePath;
    DWORD   result;
    HANDLE  fileHandle;

    ASSERT(groupId != -1);
    ASSERT(groupFile != NULL);
    TRACE("groupId=%d groupFile=%p\n", groupId, groupFile);

    // Retrieve group file location
    StringCchPrintfA(buffer, ELEMENT_COUNT(buffer), "%i", groupId);
    filePath = Io_ConfigGetPath("Locations", "Group_Files", buffer, NULL);
    if (filePath == NULL) {
        TRACE("Unable to retrieve file location.\n");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Open group file
    fileHandle = CreateFileA(filePath,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (fileHandle != INVALID_HANDLE_VALUE) {
        result = ERROR_SUCCESS;
    } else {
        result = GetLastError();
        TRACE("Unable to open file \"%s\" (error %lu).\n", filePath, result);
    }

    // Save file handle
    groupFile->lpInternal = fileHandle;

    Io_Free(filePath);
    return result;
}

DWORD FileGroupWrite(GROUPFILE *groupFile)
{
    BUFFER  buffer;
    DWORD   bytesWritten;
    DWORD   result;
    HANDLE  fileHandle;

    ASSERT(groupFile != NULL);
    TRACE("groupFile=%p\n", groupFile);

    fileHandle = groupFile->lpInternal;
    ASSERT(fileHandle != INVALID_HANDLE_VALUE);

    // Allocate write buffer
    ZeroMemory(&buffer, sizeof(BUFFER));
    buffer.dwType = TYPE_CHAR;
    buffer.size   = 4096;
    buffer.buf    = Io_Allocate(buffer.size);

    if (buffer.buf == NULL) {
        TRACE("Unable to allocate write buffer.\n");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Dump group data to buffer
    Io_GroupFile2Ascii(&buffer, groupFile);

    // Write buffer to file
    SetFilePointer(fileHandle, 0, 0, FILE_BEGIN);
    if (WriteFile(fileHandle, buffer.buf, buffer.len, &bytesWritten, NULL)) {
        result = ERROR_SUCCESS;

        // Truncate file at its current position and flush changes to disk
        SetEndOfFile(fileHandle);
        FlushFileBuffers(fileHandle);
    } else {
        result = GetLastError();
        TRACE("Unable to write file (error %lu).\n", result);
    }

    Io_Free(buffer.buf);
    return result;
}

DWORD FileGroupClose(GROUPFILE *groupFile)
{
    HANDLE fileHandle;

    ASSERT(groupFile != NULL);
    TRACE("groupFile=%p\n", groupFile);

    fileHandle = groupFile->lpInternal;

    // Close group file
    if (fileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(fileHandle);

        // Invalidate the internal pointer
        groupFile->lpInternal = INVALID_HANDLE_VALUE;
    }

    return ERROR_SUCCESS;
}
