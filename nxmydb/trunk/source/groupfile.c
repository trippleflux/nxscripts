/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2009 neoxed

Module Name:
    Group File Backend

Author:
    neoxed (neoxed@gmail.com) Jun 5, 2006

Abstract:
    Group file storage backend.

*/

#include <base.h>
#include <backends.h>

static DWORD FileGroupRead(HANDLE file, GROUPFILE *groupFile)
{
    CHAR    *buffer = NULL;
    DWORD   bytesRead;
    DWORD   fileSize;
    DWORD   result;

    ASSERT(file != INVALID_HANDLE_VALUE);
    ASSERT(groupFile != NULL);
    TRACE("file=%p groupFile=%p", file, groupFile);

    // Retrieve file size
    fileSize = GetFileSize(file, NULL);
    if (fileSize == INVALID_FILE_SIZE || fileSize < 5) {
        result = INVALID_FILE_SIZE;
        TRACE("Unable to retrieve file size (error %lu).", result);
        goto failed;
    }

    // Allocate read buffer
    buffer = Io_Allocate(fileSize + 1);
    if (buffer == NULL) {
        result = ERROR_NOT_ENOUGH_MEMORY;
        LOG_ERROR("Unable to allocate memory for read buffer.");
        goto failed;
    }

    // Read group file to buffer
    SetFilePointer(file, 0, 0, FILE_BEGIN);
    if (!ReadFile(file, buffer, fileSize, &bytesRead, NULL) || bytesRead < 5) {
        result = GetLastError();
        TRACE("Unable to read file (error %lu).", result);
        goto failed;
    }

    // Pad buffer with a new-line
    buffer[bytesRead] = '\n';
    bytesRead++;

    // Parse buffer, also initializing the GROUPFILE structure
    Io_Ascii2GroupFile(buffer, bytesRead, groupFile);

    Io_Free(buffer);
    return ERROR_SUCCESS;

failed:
    if (buffer != NULL) {
        Io_Free(buffer);
    }

    return result;
}

DWORD FileGroupDefault(GROUPFILE *groupFile)
{
    CHAR    *path;
    DWORD   result;
    HANDLE  file;

    ASSERT(groupFile != NULL);
    TRACE("groupFile=%p", groupFile);

    // Retrieve "Default.Group" location
    path = Io_ConfigGetPath(Io_ConfigGetIniFile(), "Locations", "Group_Files",
        "Default.Group", NULL);
    if (path == NULL) {
        TRACE("Unable to retrieve \"Default.Group\" file location.");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Open "Default.Group" file
    file = CreateFileA(path,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (file == INVALID_HANDLE_VALUE) {
        result = GetLastError();
        TRACE("Unable to open file \"%s\" (error %lu).", path, result);
    } else {

        // Read "Default.Group" file
        result = FileGroupRead(file, groupFile);
        if (result != ERROR_SUCCESS) {
            TRACE("Unable to read file \"%s\" (error %lu).", path, result);
        }

        // Close "Default.Group" file
        CloseHandle(file);
    }

    Io_Free(path);
    return result;
}

DWORD FileGroupCreate(INT32 groupId, GROUPFILE *groupFile)
{
    CHAR   *defaultPath = NULL;
    CHAR   *targetPath = NULL;
    CHAR   buffer[12];
    DWORD  result = ERROR_SUCCESS;

    ASSERT(groupId != -1);
    ASSERT(groupFile != NULL);
    TRACE("groupId=%d groupFile=%p", groupId, groupFile);

    // Retrieve default location
    defaultPath = Io_ConfigGetPath(Io_ConfigGetIniFile(), "Locations",
        "Group_Files", "Default.Group", NULL);
    if (defaultPath == NULL) {
        TRACE("Unable to retrieve default file location.");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Retrieve target location
    StringCchPrintfA(buffer, ELEMENT_COUNT(buffer), "%i", groupId);
    targetPath = Io_ConfigGetPath(Io_ConfigGetIniFile(), "Locations",
        "Group_Files", buffer, NULL);
    if (targetPath == NULL) {
        TRACE("Unable to retrieve group file location.");

        Io_Free(defaultPath);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Copy default file to target file
    if (!CopyFileA(defaultPath, targetPath, FALSE)) {
        result = GetLastError();
        TRACE("Unable to copy file \"%s\" to \"%s\" (error %lu).",
            defaultPath, targetPath, result);
    } else {

        // Open new group file
        result = FileGroupOpen(groupId, groupFile);
        if (result != ERROR_SUCCESS) {
            TRACE("Unable to open group file (error %lu).", result);
        }
    }

    Io_Free(defaultPath);
    Io_Free(targetPath);

    return result;
}

DWORD FileGroupDelete(INT32 groupId)
{
    CHAR    *path;
    CHAR    buffer[12];
    DWORD   result;

    ASSERT(groupId != -1);
    TRACE("groupId=%d", groupId);

    // Retrieve group file location
    StringCchPrintfA(buffer, ELEMENT_COUNT(buffer), "%i", groupId);
    path = Io_ConfigGetPath(Io_ConfigGetIniFile(), "Locations", "Group_Files",
        buffer, NULL);
    if (path == NULL) {
        TRACE("Unable to retrieve file location.");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Delete group file and free resources
    if (DeleteFileA(path)) {
        result = ERROR_SUCCESS;
    } else {
        result = GetLastError();
        TRACE("Unable to delete file \"%s\" (error %lu).", path, result);
    }

    Io_Free(path);
    return result;
}

DWORD FileGroupOpen(INT32 groupId, GROUPFILE *groupFile)
{
    CHAR        buffer[12];
    CHAR        *path;
    DWORD       result;
    MOD_CONTEXT *mod;

    ASSERT(groupId != -1);
    ASSERT(groupFile != NULL);
    TRACE("groupId=%d groupFile=%p", groupId, groupFile);

    // Check if ioFTPD wiped out the module context pointer
    ASSERT(groupFile->lpInternal != NULL);
    mod = groupFile->lpInternal;

    // There must not be an existing file handle
    ASSERT(mod->file == INVALID_HANDLE_VALUE);

    // Retrieve group file location
    StringCchPrintfA(buffer, ELEMENT_COUNT(buffer), "%i", groupId);
    path = Io_ConfigGetPath(Io_ConfigGetIniFile(), "Locations", "Group_Files",
        buffer, NULL);
    if (path == NULL) {
        TRACE("Unable to retrieve file location.");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Open group file
    mod->file = CreateFileA(path,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (mod->file != INVALID_HANDLE_VALUE) {
        result = ERROR_SUCCESS;
    } else {
        result = GetLastError();
        TRACE("Unable to open file \"%s\" (error %lu).", path, result);
    }

    Io_Free(path);
    return result;
}

DWORD FileGroupWrite(GROUPFILE *groupFile)
{
    BUFFER      buffer;
    DWORD       bytesWritten;
    DWORD       result;
    MOD_CONTEXT *mod;

    ASSERT(groupFile != NULL);
    TRACE("groupFile=%p", groupFile);

    // Check if ioFTPD wiped out the module context pointer
    ASSERT(groupFile->lpInternal != NULL);
    mod = groupFile->lpInternal;

    // There must be an existing file handle
    ASSERT(mod->file != INVALID_HANDLE_VALUE);

    // Allocate write buffer
    ZeroMemory(&buffer, sizeof(BUFFER));
    buffer.dwType = TYPE_CHAR;
    buffer.size   = 4096;
    buffer.buf    = Io_Allocate(buffer.size);

    if (buffer.buf == NULL) {
        LOG_ERROR("Unable to allocate memory for write buffer.");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Dump group data to buffer
    Io_GroupFile2Ascii(&buffer, groupFile);

    // Write buffer to file
    SetFilePointer(mod->file, 0, 0, FILE_BEGIN);
    if (WriteFile(mod->file, buffer.buf, buffer.len, &bytesWritten, NULL)) {
        result = ERROR_SUCCESS;

        // Truncate file at its current position and flush changes to disk
        SetEndOfFile(mod->file);
        FlushFileBuffers(mod->file);
    } else {
        result = GetLastError();
        TRACE("Unable to write file (error %lu).", result);
    }

    Io_Free(buffer.buf);
    return result;
}

DWORD FileGroupClose(GROUPFILE *groupFile)
{
    MOD_CONTEXT *mod;

    ASSERT(groupFile != NULL);
    TRACE("groupFile=%p", groupFile);

    mod = groupFile->lpInternal;

    // Close group file
    if (mod->file != INVALID_HANDLE_VALUE) {
        CloseHandle(mod->file);

        // Invalidate the internal pointer
        mod->file = INVALID_HANDLE_VALUE;
    }

    return ERROR_SUCCESS;
}
