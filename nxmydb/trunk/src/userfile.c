/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    User File Backend

Author:
    neoxed (neoxed@gmail.com) Jun 5, 2006

Abstract:
    User file storage backend.

*/

#include "mydb.h"

static
BOOL
UserRead(
    char *filePath,
    USERFILE *userFile
    )
{
    char *buffer = NULL;
    DWORD bytesRead;
    DWORD error;
    DWORD fileSize;
    USER_CONTEXT *context;

    ASSERT(filePath != NULL);
    ASSERT(userFile != NULL);
    ASSERT(userFile->lpInternal != NULL);
    DebugPrint("FileUserRead", "filePath=\"%s\" userFile=%p\n", filePath, userFile);

    context = userFile->lpInternal;

    // Open the user file
    context->fileHandle = CreateFileA(filePath,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (context->fileHandle == INVALID_HANDLE_VALUE) {
        DebugPrint("FileUserRead", "Unable to open file (error %lu).\n", GetLastError());
        return FALSE;
    }

    // Retrieve file size
    fileSize = GetFileSize(context->fileHandle, NULL);
    if (fileSize == INVALID_FILE_SIZE || fileSize < 5) {
        DebugPrint("FileUserRead", "Unable to retrieve file size, or file size is under 5 bytes.\n");
        goto failed;
    }

    // Allocate read buffer
    buffer = Io_Allocate(fileSize + 1);
    if (buffer == NULL) {
        DebugPrint("FileUserRead", "Unable to allocate read buffer.\n");
        goto failed;
    }

    // Read user file to buffer
    if (!ReadFile(context->fileHandle, buffer, fileSize, &bytesRead, NULL) || bytesRead < 5) {
        DebugPrint("FileUserRead", "Unable to read file, or the amount read is under 5 bytes.\n");
        goto failed;
    }

    // Pad buffer with a new-line
    buffer[bytesRead] = '\n';
    bytesRead++;

    // Parse buffer, also initializing the USERFILE structure
    Io_Ascii2UserFile(buffer, bytesRead, userFile);
    userFile->Gid = userFile->Groups[0];

    Io_Free(buffer);
    return TRUE;

failed:
    // Preserve system error code
    error = GetLastError();

    if (buffer != NULL) {
        Io_Free(buffer);
    }
    CloseHandle(context->fileHandle);

    // Restore system error code
    SetLastError(error);
    return FALSE;
}

BOOL
FileUserCreate(
    INT32 userId,
    USERFILE *userFile
    )
{
    char *defaultPath;
    char *targetPath;
    char buffer[12];
    DWORD error;

    ASSERT(userFile != NULL);
    DebugPrint("FileUserCreate", "userId=%i userFile=%p\n", userId, userFile);

    // Retrieve default user location
    defaultPath = Io_ConfigGetPath("Locations", "User_Files", "Default.User", NULL);
    if (defaultPath == NULL) {
        DebugPrint("FileUserCreate", "Unable to retrieve default file location.\n");

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    // Retrieve user location
    StringCchPrintfA(buffer, ARRAYSIZE(buffer), "%i", userId);
    targetPath = Io_ConfigGetPath("Locations", "User_Files", buffer, NULL);
    if (targetPath == NULL) {
        DebugPrint("FileUserCreate", "Unable to retrieve file location.\n");

        Io_Free(defaultPath);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    // Copy default file to target file
    if (!CopyFileA(defaultPath, targetPath, FALSE)) {
        DebugPrint("FileUserCreate", "Unable to copy default file (error %lu).\n", GetLastError());
        goto failed;
    }

    // Read user file (copy of "Default.User")
    if (!UserRead(targetPath, userFile)) {
        DebugPrint("FileUserCreate", "Unable read target file (error %lu).\n", GetLastError());
        goto failed;
    }

    Io_Free(defaultPath);
    Io_Free(targetPath);
    return TRUE;

failed:
    // Preserve system error code
    error = GetLastError();

    Io_Free(defaultPath);
    Io_Free(targetPath);

    // Restore system error code
    SetLastError(error);
    return FALSE;
}

BOOL
FileUserDelete(
    INT32 userId
    )
{
    char buffer[12];
    char *filePath;

    DebugPrint("FileUserDelete", "userId=%i\n", userId);

    // Retrieve user file location
    StringCchPrintfA(buffer, ARRAYSIZE(buffer), "%i", userId);
    filePath = Io_ConfigGetPath("Locations", "User_Files", buffer, NULL);
    if (filePath == NULL) {
        DebugPrint("FileUserDelete", "Unable to retrieve file location.\n");

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    // Delete user file and free resources
    DeleteFileA(filePath);
    Io_Free(filePath);

    return TRUE;
}

BOOL
FileUserOpen(
    INT32 userId,
    USER_CONTEXT *context
    )
{
    char buffer[12];
    char *filePath;
    DWORD error;

    ASSERT(context != NULL);
    DebugPrint("FileUserOpen", "userId=%i context=%p\n", userId, context);

    // Retrieve user file location
    StringCchPrintfA(buffer, ARRAYSIZE(buffer), "%i", userId);
    filePath = Io_ConfigGetPath("Locations", "User_Files", buffer, NULL);
    if (filePath == NULL) {
        DebugPrint("FileUserOpen", "Unable to retrieve file location.\n");

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    // Open user file
    context->fileHandle = CreateFileA(filePath,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (context->fileHandle == INVALID_HANDLE_VALUE) {
        // Preserve system error code
        error = GetLastError();
        Io_Free(filePath);

        DebugPrint("FileUserOpen", "Unable to open file (error %lu).\n", error);

        // Restore system error code
        SetLastError(error);
        return FALSE;
    }

    Io_Free(filePath);
    return TRUE;
}

BOOL
FileUserWrite(
    USERFILE *userFile
    )
{
    BUFFER buffer;
    DWORD bytesWritten;
    DWORD error;
    USER_CONTEXT *context;

    ASSERT(userFile != NULL);
    ASSERT(userFile->lpInternal != NULL);
    DebugPrint("FileUserWrite", "userFile=%p\n", userFile);

    context = userFile->lpInternal;

    // Allocate write buffer
    ZeroMemory(&buffer, sizeof(BUFFER));
    buffer.dwType = TYPE_CHAR;
    buffer.size   = 4096;
    buffer.buf    = Io_Allocate(buffer.size);

    if (buffer.buf == NULL) {
        DebugPrint("FileUserWrite", "Unable to allocate write buffer.\n");

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    // Dump user data to buffer
    Io_UserFile2Ascii(&buffer, userFile);

    // Write buffer to file
    SetFilePointer(context->fileHandle, 0, 0, FILE_BEGIN);
    if (!WriteFile(context->fileHandle, buffer.buf, buffer.len, &bytesWritten, NULL)) {
        // Preserve system error code
        error = GetLastError();
        Io_Free(buffer.buf);

        DebugPrint("FileUserWrite", "Unable to write file (error %lu).\n", error);

        // Restore system error code
        SetLastError(error);
        return FALSE;
    }

    // Truncate file at its current position and flush changes to disk
    SetEndOfFile(context->fileHandle);
    FlushFileBuffers(context->fileHandle);

    Io_Free(buffer.buf);
    return TRUE;
}

BOOL
FileUserClose(
    USER_CONTEXT *context
    )
{
    ASSERT(context != NULL);
    DebugPrint("FileUserClose", "context=%p\n", context);

    // Close user file handle
    ASSERT(context->fileHandle != INVALID_HANDLE_VALUE);
    CloseHandle(context->fileHandle);

    return TRUE;
}
