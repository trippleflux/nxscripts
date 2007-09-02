/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    User File Backend

Author:
    neoxed (neoxed@gmail.com) Jun 5, 2006

Abstract:
    User file storage backend.

*/

#include <base.h>
#include <backends.h>

static DWORD FileUserRead(CHAR *filePath, USERFILE *userFile)
{
    CHAR        *buffer = NULL;
    DWORD       bytesRead;
    DWORD       fileSize;
    DWORD       result;
    MOD_CONTEXT *mod;

    ASSERT(filePath != NULL);
    ASSERT(userFile != NULL);
    TRACE("filePath=%s userFile=%p\n", filePath, userFile);

    mod = userFile->lpInternal;
    ASSERT(mod->file == INVALID_HANDLE_VALUE);

    // Open user file
    mod->file = CreateFileA(filePath,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (mod->file == INVALID_HANDLE_VALUE) {
        result = GetLastError();
        TRACE("Unable to open file \"%s\" (error %lu).\n", filePath, result);
        goto failed;
    }

    // Retrieve file size
    fileSize = GetFileSize(mod->file, NULL);
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

    // Read user file to buffer
    if (!ReadFile(mod->file, buffer, fileSize, &bytesRead, NULL) || bytesRead < 5) {
        result = GetLastError();
        TRACE("Unable to write file \"%s\" (error %lu).\n", filePath, result);
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
    if (mod->file != INVALID_HANDLE_VALUE) {
        CloseHandle(mod->file);
    }
    if (buffer != NULL) {
        Io_Free(buffer);
    }

    return result;
}

DWORD FileUserCreate(INT32 userId, USERFILE *userFile)
{
    CHAR  *defaultPath = NULL;
    CHAR  *targetPath = NULL;
    CHAR  buffer[12];
    DWORD result;

    ASSERT(userFile != NULL);
    TRACE("userId=%d userFile=%p\n", userId, userFile);

    // Retrieve default user location
    defaultPath = Io_ConfigGetPath("Locations", "User_Files", "Default.User", NULL);
    if (defaultPath == NULL) {
        TRACE("Unable to retrieve default file location.\n");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Retrieve user location
    StringCchPrintfA(buffer, ELEMENT_COUNT(buffer), "%i", userId);
    targetPath = Io_ConfigGetPath("Locations", "User_Files", buffer, NULL);
    if (targetPath == NULL) {
        TRACE("Unable to retrieve file location.\n");
        result = ERROR_NOT_ENOUGH_MEMORY;
    } else {

        // Copy default file to target file
        if (!CopyFileA(defaultPath, targetPath, FALSE)) {
            result = GetLastError();
            TRACE("Unable to copy default file (error %lu).\n", result);

        } else {
            // Read user file (copy of "Default.User")
            result = FileUserRead(targetPath, userFile);
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

DWORD FileUserDelete(INT32 userId)
{
    CHAR  buffer[12];
    CHAR  *filePath;
    DWORD result;

    ASSERT(userId != -1);
    TRACE("userId=%d\n", userId);

    // Retrieve user file location
    StringCchPrintfA(buffer, ELEMENT_COUNT(buffer), "%i", userId);
    filePath = Io_ConfigGetPath("Locations", "User_Files", buffer, NULL);
    if (filePath == NULL) {
        TRACE("Unable to retrieve file location.\n");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Delete user file and free resources
    if (DeleteFileA(filePath)) {
        result = ERROR_SUCCESS;
    } else {
        result = GetLastError();
        TRACE("Unable to delete file \"%s\" (error %lu).\n", filePath, result);
    }

    Io_Free(filePath);
    return result;
}

DWORD FileUserOpen(INT32 userId, USERFILE *userFile)
{
    CHAR        buffer[12];
    CHAR        *filePath;
    DWORD       result;
    MOD_CONTEXT *mod;

    ASSERT(userId != -1);
    ASSERT(userFile != NULL);
    TRACE("userId=%d userFile=%p\n", userId, userFile);

    mod = userFile->lpInternal;
    ASSERT(mod->file == INVALID_HANDLE_VALUE);

    // Retrieve user file location
    StringCchPrintfA(buffer, ELEMENT_COUNT(buffer), "%i", userId);
    filePath = Io_ConfigGetPath("Locations", "User_Files", buffer, NULL);
    if (filePath == NULL) {
        TRACE("Unable to retrieve file location.\n");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Open user file
    mod->file = CreateFileA(filePath,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (mod->file != INVALID_HANDLE_VALUE) {
        result = ERROR_SUCCESS;
    } else {
        result = GetLastError();
        TRACE("Unable to open file \"%s\" (error %lu).\n", filePath, result);
    }

    Io_Free(filePath);
    return result;
}

DWORD FileUserWrite(USERFILE *userFile)
{
    BUFFER      buffer;
    DWORD       bytesWritten;
    DWORD       result;
    MOD_CONTEXT *mod;

    ASSERT(userFile != NULL);
    TRACE("userFile=%p\n", userFile);

    mod = userFile->lpInternal;
    ASSERT(mod->file != INVALID_HANDLE_VALUE);

    // Allocate write buffer
    ZeroMemory(&buffer, sizeof(BUFFER));
    buffer.dwType = TYPE_CHAR;
    buffer.size   = 4096;
    buffer.buf    = Io_Allocate(buffer.size);

    if (buffer.buf == NULL) {
        TRACE("Unable to allocate write buffer.\n");
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
        TRACE("Unable to write file (error %lu).\n", result);
    }

    Io_Free(buffer.buf);
    return result;
}

DWORD FileUserClose(USERFILE *userFile)
{
    MOD_CONTEXT *mod;

    ASSERT(userFile != NULL);
    TRACE("userFile=%p\n", userFile);

    mod = userFile->lpInternal;

    // Close user file
    if (mod->file != INVALID_HANDLE_VALUE) {
        CloseHandle(mod->file);

        // Invalidate the internal pointer
        mod->file = INVALID_HANDLE_VALUE;
    }

    return ERROR_SUCCESS;
}
