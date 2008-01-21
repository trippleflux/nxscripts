/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2008 neoxed

Module Name:
    User File Backend

Author:
    neoxed (neoxed@gmail.com) Jun 5, 2006

Abstract:
    User file storage backend.

*/

#include <base.h>
#include <backends.h>

static DWORD FileUserRead(HANDLE file, USERFILE *userFile)
{
    CHAR    *buffer = NULL;
    DWORD   bytesRead;
    DWORD   fileSize;
    DWORD   result;

    ASSERT(file != INVALID_HANDLE_VALUE);
    ASSERT(userFile != NULL);
    TRACE("file=%p userFile=%p", file, userFile);

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
        TRACE("Unable to allocate read buffer.");
        goto failed;
    }

    // Read user file to buffer
    SetFilePointer(file, 0, 0, FILE_BEGIN);
    if (!ReadFile(file, buffer, fileSize, &bytesRead, NULL) || bytesRead < 5) {
        result = GetLastError();
        TRACE("Unable to read file (error %lu).", result);
        goto failed;
    }

    // Pad buffer with a new-line
    buffer[bytesRead] = '\n';
    bytesRead++;

    // Parse buffer, also initializing the USERFILE structure
    Io_Ascii2UserFile(buffer, bytesRead, userFile);
    userFile->Gid = userFile->Groups[0];

    Io_Free(buffer);
    return ERROR_SUCCESS;

failed:
    if (buffer != NULL) {
        Io_Free(buffer);
    }

    return result;
}

DWORD FileUserDefault(USERFILE *userFile)
{
    CHAR    *path;
    DWORD   result;
    HANDLE  file;

    ASSERT(userFile != NULL);
    TRACE("userFile=%p", userFile);

    // Retrieve "Default.User" location
    path = Io_ConfigGetPath("Locations", "User_Files", "Default.User", NULL);
    if (path == NULL) {
        TRACE("Unable to retrieve \"Default.User\" file location.");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Open "Default.User" file
    file = CreateFileA(path,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (file == INVALID_HANDLE_VALUE) {
        result = GetLastError();
        TRACE("Unable to open file \"%s\" (error %lu).", path, result);
    } else {

        // Read "Default.User" file
        result = FileUserRead(file, userFile);
        if (result != ERROR_SUCCESS) {
            TRACE("Unable to read file \"%s\" (error %lu).", path, result);
        }

        // Close "Default.User" file
        CloseHandle(file);
    }

    Io_Free(path);
    return result;
}

DWORD FileUserCreate(INT32 userId, USERFILE *userFile)
{
    CHAR   *defaultPath = NULL;
    CHAR   *targetPath = NULL;
    CHAR   buffer[12];
    DWORD  result = ERROR_SUCCESS;

    ASSERT(userId != -1);
    ASSERT(userFile != NULL);
    TRACE("userId=%d userFile=%p", userId, userFile);

    // Retrieve default location
    defaultPath = Io_ConfigGetPath("Locations", "User_Files", "Default.User", NULL);
    if (defaultPath == NULL) {
        TRACE("Unable to retrieve default file location.");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Retrieve target location
    StringCchPrintfA(buffer, ELEMENT_COUNT(buffer), "%i", userId);
    targetPath = Io_ConfigGetPath("Locations", "User_Files", buffer, NULL);
    if (targetPath == NULL) {
        TRACE("Unable to retrieve user file location.");

        Io_Free(defaultPath);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Copy default file to target file
    if (!CopyFileA(defaultPath, targetPath, FALSE)) {
        result = GetLastError();
        TRACE("Unable to copy file \"%s\" to \"%s\" (error %lu).",
            defaultPath, targetPath, result);
    } else {

        // Open new user file
        result = FileUserOpen(userId, userFile);
        if (result != ERROR_SUCCESS) {
            TRACE("Unable to open user file (error %lu).", result);
        }
    }

    Io_Free(defaultPath);
    Io_Free(targetPath);

    return result;
}

DWORD FileUserDelete(INT32 userId)
{
    CHAR    *path;
    CHAR    buffer[12];
    DWORD   result;

    ASSERT(userId != -1);
    TRACE("userId=%d", userId);

    // Retrieve user file location
    StringCchPrintfA(buffer, ELEMENT_COUNT(buffer), "%i", userId);
    path = Io_ConfigGetPath("Locations", "User_Files", buffer, NULL);
    if (path == NULL) {
        TRACE("Unable to retrieve file location.");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Delete user file and free resources
    if (DeleteFileA(path)) {
        result = ERROR_SUCCESS;
    } else {
        result = GetLastError();
        TRACE("Unable to delete file \"%s\" (error %lu).", path, result);
    }

    Io_Free(path);
    return result;
}

DWORD FileUserOpen(INT32 userId, USERFILE *userFile)
{
    CHAR        buffer[12];
    CHAR        *path;
    DWORD       result;
    MOD_CONTEXT *mod;

    ASSERT(userId != -1);
    ASSERT(userFile != NULL);
    TRACE("userId=%d userFile=%p", userId, userFile);

    mod = userFile->lpInternal;
    ASSERT(mod->file == INVALID_HANDLE_VALUE);

    // Retrieve user file location
    StringCchPrintfA(buffer, ELEMENT_COUNT(buffer), "%i", userId);
    path = Io_ConfigGetPath("Locations", "User_Files", buffer, NULL);
    if (path == NULL) {
        TRACE("Unable to retrieve file location.");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Open user file
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

DWORD FileUserWrite(USERFILE *userFile)
{
    BUFFER      buffer;
    DWORD       bytesWritten;
    DWORD       result;
    MOD_CONTEXT *mod;

    ASSERT(userFile != NULL);
    TRACE("userFile=%p", userFile);

    mod = userFile->lpInternal;
    ASSERT(mod->file != INVALID_HANDLE_VALUE);

    // Allocate write buffer
    ZeroMemory(&buffer, sizeof(BUFFER));
    buffer.dwType = TYPE_CHAR;
    buffer.size   = 4096;
    buffer.buf    = Io_Allocate(buffer.size);

    if (buffer.buf == NULL) {
        TRACE("Unable to allocate write buffer.");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Dump user data to buffer
    Io_UserFile2Ascii(&buffer, userFile);

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

DWORD FileUserClose(USERFILE *userFile)
{
    MOD_CONTEXT *mod;

    ASSERT(userFile != NULL);
    TRACE("userFile=%p", userFile);

    mod = userFile->lpInternal;

    // Close user file
    if (mod->file != INVALID_HANDLE_VALUE) {
        CloseHandle(mod->file);

        // Invalidate the internal pointer
        mod->file = INVALID_HANDLE_VALUE;
    }

    return ERROR_SUCCESS;
}
