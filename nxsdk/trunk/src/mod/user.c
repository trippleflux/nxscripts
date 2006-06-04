/*

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    User Module

Author:
    neoxed (neoxed@gmail.com) Jun 4, 2006

Abstract:
    Entry point and functions for user modules.

*/

#include "mod.h"

//
// Function and type declarations
//

static INT   MODULE_CALL UserFinalize(void);
static INT32 MODULE_CALL UserCreate(char *userName);
static INT   MODULE_CALL UserRename(char *userName, INT32 userId, char *newName);
static INT   MODULE_CALL UserDelete(char *userName, INT32 userId);
static INT   MODULE_CALL UserLock(USERFILE *userFile);
static INT   MODULE_CALL UserUnlock(USERFILE *userFile);
static INT   MODULE_CALL UserRead(char *filePath, USERFILE *userFile);
static INT   MODULE_CALL UserWrite(USERFILE *userFile);
static INT   MODULE_CALL UserOpen(char *userName, USERFILE *userFile);
static INT   MODULE_CALL UserClose(USERFILE *userFile);

typedef struct {
    HANDLE fileHandle;
    volatile LONG locked;
} USER_CONTEXT;

//
// Local variables
//

static USER_MODULE *userModule = NULL;


INT
MODULE_CALL
UserModuleInit(
    USER_MODULE *module
    )
{
    DebugPrint("UserInit: module=%p\n", module);

    // Initialize module structure
    module->tszModuleName = "NXMOD";
    module->DeInitialize  = UserFinalize;
    module->Create        = UserCreate;
    module->Delete        = UserDelete;
    module->Rename        = UserRename;
    module->Lock          = UserLock;
    module->Write         = UserWrite;
    module->Open          = UserOpen;
    module->Close         = UserClose;
    module->Unlock        = UserUnlock;

    // Initialize procedure table
    if (!InitProcTable(module->GetProc)) {
        DebugPrint("UserInit: Unable to initialize procedure table.\n");
        return UM_ERROR;
    }
    Io_Putlog(LOG_ERROR, "nxMod user module loaded.\r\n");

    userModule = module;
    return UM_SUCCESS;
}

static
INT
MODULE_CALL
UserFinalize(
    void
    )
{
    DebugPrint("UserFinalize: module=%p\n", userModule);
    Io_Putlog(LOG_ERROR, "nxMod user module unloaded.\r\n");

    // Finalize procedure table
    FinalizeProcTable();
    userModule = NULL;
    return UM_SUCCESS;
}

static
INT32
MODULE_CALL
UserCreate(
    char *userName
    )
{
    char buffer[_MAX_NAME + 12];
    char *sourcePath;
    char *tempPath;
    DWORD error;
    INT32 result;
    USERFILE userFile;

    DebugPrint("UserCreate: userName=\"%s\"\n", userName);

    // Retrieve default location
    sourcePath = Io_ConfigGetPath("Locations", "User_Files", "Default.User", NULL);
    if (sourcePath == NULL) {
        DebugPrint("UserCreate: Unable to retrieve default file location.\n");

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return -1;
    }

    // Retrieve temporary location
    StringCchPrintfA(buffer, ARRAYSIZE(buffer), "%s.tmp", userName);
    tempPath = Io_ConfigGetPath("Locations", "User_Files", buffer, NULL);
    if (tempPath == NULL) {
        DebugPrint("UserCreate: Unable to retrieve temporary file location.\n");

        // Free resources
        Io_Free(sourcePath);

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return -1;
    }

    // Copy default file
    if (!CopyFileA(sourcePath, tempPath, FALSE)) {
        error = GetLastError();
        DebugPrint("UserCreate: Unable to copy default file (error %lu).\n", error);

        // Free resources
        Io_Free(sourcePath);
        Io_Free(tempPath);

        // Restore system error code
        SetLastError(error);
        return -1;
    }

    // Initialize USERFILE structure
    ZeroMemory(&userFile, sizeof(USERFILE));
    userFile.Groups[0]      = NOGROUP_ID;
    userFile.Groups[1]      = -1;
    userFile.AdminGroups[0] = -1;

    if (UserRead(tempPath, &userFile) != UM_SUCCESS) {
        error = GetLastError();

        // Free resources
        DeleteFileA(tempPath);
        Io_Free(sourcePath);
        Io_Free(tempPath);

        // Restore system error code
        SetLastError(error);
        return -1;
    }

    // Register user
    result = userModule->Register(userModule, userName, &userFile);
    if (result == -1) {
        error = GetLastError();

        UserClose(&userFile);
        DeleteFileA(tempPath);
    } else {
        char *offset;
        size_t sourceLength = strlen(sourcePath);

        // Terminate string after the last path separator
        offset = strrchr(sourcePath, '\\');
        if (offset != NULL) {
            offset[1] = '\0';
        } else {
            offset = strrchr(sourcePath, '/');
            if (offset != NULL) {
                offset[1] = '\0';
            }
        }

        // Append the user's ID
        StringCchPrintfA(buffer, ARRAYSIZE(buffer), "%i", result);
        StringCchCatA(sourcePath, sourceLength, buffer);

        // Rename temporary file
        if (!MoveFileExA(tempPath, sourcePath, MOVEFILE_REPLACE_EXISTING)) {
            error = GetLastError();
            DebugPrint("UserCreate: Unable to rename temporary file (error %lu).\n", error);

            // Unregister user and delete the data file
            if (!userModule->Unregister(userModule, userName)) {
                UserClose(&userFile);
            }
            DeleteFileA(tempPath);

            result = -1;
        } else {
            error = ERROR_SUCCESS;
        }
    }

    // Free resources
    Io_Free(sourcePath);
    Io_Free(tempPath);

    // Restore system error code
    SetLastError(error);
    return result;
}

static
INT
MODULE_CALL
UserRename(
    char *userName,
    INT32 userId,
    char *newName
    )
{
    DebugPrint("UserRename: userName=\"%s\" userId=%i newName=\"%s\"\n", userName, userId, newName);

    // Register the user under the new name
    if (userModule->RegisterAs(userModule, userName, newName)) {
        DebugPrint("UserRename: Unable to rename user, already exists?\n");
        return UM_ERROR;
    }

    return UM_SUCCESS;
}

static
INT
MODULE_CALL
UserDelete(
    char *userName,
    INT32 userId
    )
{
    char buffer[16];
    char *filePath;

    DebugPrint("UserDelete: userName=\"%s\" userId=%i\n", userName, userId);

    // Format user ID
    StringCchPrintfA(buffer, ARRAYSIZE(buffer), "%i", userId);

    // Retrieve file location
    filePath = Io_ConfigGetPath("Locations", "User_Files", buffer, NULL);
    if (filePath == NULL) {
        DebugPrint("UserDelete: Unable to retrieve file location.\n");

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return UM_ERROR;
    }

    // Delete data file and free resources
    DeleteFileA(filePath);
    Io_Free(filePath);

    // Unregister user
    if (userModule->Unregister(userModule, userName)) {
        DebugPrint("UserDelete: Unable to unregister user.\n");
        return UM_ERROR;
    }

    return UM_SUCCESS;
}

static
INT
MODULE_CALL
UserLock(
    USERFILE *userFile
    )
{
    USER_CONTEXT *context;
    DebugPrint("UserLock: userFile=%p\n", userFile);

    // Actual implementations should use a proper locking mechanism.
    context = (USER_CONTEXT *)userFile->lpInternal;
    if (InterlockedCompareExchange(&context->locked, 1, 0) == 1) {
        DebugPrint("UserLock: Unable to aquire lock.\n");
        return UM_ERROR;
    }

    return UM_SUCCESS;
}

static
INT
UserUnlock(
    USERFILE *userFile
    )
{
    USER_CONTEXT *context;
    DebugPrint("UserUnlock: userFile=%p\n", userFile);

    // Clear locked flag.
    context = (USER_CONTEXT *)userFile->lpInternal;
    context->locked = 0;

    return UM_SUCCESS;
}

static
INT
MODULE_CALL
UserRead(
    char *filePath,
    USERFILE *userFile
    )
{
    char *buffer = NULL;
    DWORD bytesRead;
    DWORD error;
    DWORD fileSize;
    INT result = UM_FATAL;
    USER_CONTEXT *context;

    DebugPrint("UserRead: filePath=\"%s\" userFile=%p\n", filePath, userFile);

    // Allocate user context
    context = (USER_CONTEXT *)Io_Allocate(sizeof(USER_CONTEXT));
    if (context == NULL) {
        DebugPrint("UserRead: Unable to allocate user context.\n");

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return UM_FATAL;
    }
    context->locked = 0;

    // Open the user's data file
    context->fileHandle = CreateFileA(filePath,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (context->fileHandle == INVALID_HANDLE_VALUE) {
        result = (GetLastError() == ERROR_FILE_NOT_FOUND) ? UM_DELETED : UM_FATAL;
        DebugPrint("UserRead: Unable to open file (error %lu).\n", GetLastError());
        goto end;
    }

    // Retrieve file size
    fileSize = GetFileSize(context->fileHandle, NULL);
    if (fileSize == INVALID_FILE_SIZE || fileSize < 5) {
        DebugPrint("UserRead: Unable to retrieve file size, or file size is under 5 bytes.\n");
        goto end;
    }

    // Allocate read buffer
    buffer = (char *)Io_Allocate(fileSize + 1);
    if (buffer == NULL) {
        DebugPrint("UserRead: Unable to allocate read buffer.\n");
        goto end;
    }

    // Read data file to buffer
    if (!ReadFile(context->fileHandle, buffer, fileSize, &bytesRead, NULL) || bytesRead < 5) {
        DebugPrint("UserRead: Unable to read file, or the amount read is under 5 bytes.\n");
        goto end;
    }

    // Pad buffer with a new-line
    buffer[bytesRead] = '\n';
    bytesRead++;

    // Parse buffer, initializing the USERFILE structure
    Io_Ascii2UserFile(buffer, bytesRead, userFile);
    userFile->Gid        = userFile->Groups[0];
    userFile->lpInternal = context;
    result               = UM_SUCCESS;

end:
    // Free objects and resources
    if (result != UM_SUCCESS) {
        error = GetLastError();
        if (context->fileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(context->fileHandle);
        }
        Io_Free(buffer);
        Io_Free(context);
        userFile->lpInternal = NULL;

        // Restore system error code
        SetLastError(error);
    } else {
        Io_Free(buffer);
    }

    return result;
}

static
INT
MODULE_CALL
UserWrite(
    USERFILE *userFile
    )
{
    BUFFER buffer;
    DWORD bytesWritten;
    DWORD error;
    USER_CONTEXT *context;

    DebugPrint("UserWrite: userFile=%p\n", userFile);

    // Retrieve user context
    context = (USER_CONTEXT *)userFile->lpInternal;

    // Allocate write buffer
    ZeroMemory(&buffer, sizeof(BUFFER));
    buffer.dwType = TYPE_CHAR;
    buffer.size   = 4096;
    buffer.buf    = (char *)Io_Allocate(buffer.size);

    if (buffer.buf == NULL) {
        DebugPrint("UserWrite: Unable to allocate write buffer.\n");

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return UM_ERROR;
    }

    // Dump user data to buffer
    Io_UserFile2Ascii(&buffer, userFile);

    // Write buffer to file
    SetFilePointer(context->fileHandle, 0, 0, FILE_BEGIN);
    if (!WriteFile(context->fileHandle, buffer.buf, buffer.len, &bytesWritten, NULL)) {
        error = GetLastError();
        DebugPrint("UserWrite: Unable to write file (error %lu).\n", error);

        // Free resources
        Io_Free(buffer.buf);

        // Restore system error code
        SetLastError(error);
        return UM_ERROR;
    }

    // Truncate file at its current position and flush changes to disk
    SetEndOfFile(context->fileHandle);
    FlushFileBuffers(context->fileHandle);

    Io_Free(buffer.buf);
    return UM_SUCCESS;
}

static
INT
MODULE_CALL
UserOpen(
    char *userName,
    USERFILE *userFile
    )
{
    char buffer[16];
    char *filePath;
    INT result;

    DebugPrint("UserOpen: userName=\"%s\" userFile=%p\n", userName, userFile);

    // Format user ID
    StringCchPrintfA(buffer, ARRAYSIZE(buffer), "%i", userFile->Uid);

    // Retrieve file location
    filePath = Io_ConfigGetPath("Locations", "User_Files", buffer, NULL);
    if (filePath == NULL) {
        DebugPrint("UserOpen: Unable to retrieve file location.\n");
        return UM_FATAL;
    }

    // Read the user's data file
    result = UserRead(filePath, userFile);
    Io_Free(filePath);

    return result;
}

static
INT
MODULE_CALL
UserClose(
    USERFILE *userFile
    )
{
    USER_CONTEXT *context;

    DebugPrint("UserClose: userFile=%p\n", userFile);

    // Retrieve user context
    context = (USER_CONTEXT *)userFile->lpInternal;
    if (context == NULL) {
        DebugPrint("UserClose: User context already freed.\n");
        return UM_ERROR;
    }

    // Free objects and resources
    CloseHandle(context->fileHandle);
    Io_Free(context);
    userFile->lpInternal = NULL;

    return UM_SUCCESS;
}
