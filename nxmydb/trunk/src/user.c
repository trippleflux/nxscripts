/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    User Module

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Entry point and functions for user modules.

*/

#include "mydb.h"

//
// Function and type declarations
//

static BOOL  MODULE_CALL UserFinalize(void);
static INT32 MODULE_CALL UserCreate(char *userName);
static BOOL  MODULE_CALL UserRename(char *userName, INT32 userId, char *newName);
static BOOL  MODULE_CALL UserDelete(char *userName, INT32 userId);
static BOOL  MODULE_CALL UserLock(USERFILE *userFile);
static BOOL  MODULE_CALL UserUnlock(USERFILE *userFile);
static INT   MODULE_CALL UserRead(char *filePath, USERFILE *userFile);
static INT   MODULE_CALL UserWrite(USERFILE *userFile);
static INT   MODULE_CALL UserOpen(char *userName, USERFILE *userFile);
static BOOL  MODULE_CALL UserClose(USERFILE *userFile);

typedef struct {
    HANDLE fileHandle;
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
    module->tszModuleName = "NXMYDB";
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
        DebugPrint("UserInit: Unable to initialize the procedure table.\n");
        return 1;
    }
    Io_Putlog(LOG_ERROR, "nxMyDB user module loaded.\r\n");

    userModule = module;
    return 0;
}

static
INT
MODULE_CALL
UserFinalize(
    void
    )
{
    DebugPrint("UserFinalize: module=%p\n", userModule);
    Io_Putlog(LOG_ERROR, "nxMyDB user module unloaded.\r\n");

    // Finalize procedure table
    FinalizeProcTable();
    userModule = NULL;
    return 0;
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
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);

        DebugPrint("UserCreate: Unable to retrieve default file location.\n");
        return -1;
    }

    // Retrieve temporary location
    StringCchPrintfA(buffer, ARRAYSIZE(buffer), "%s.temp", userName);
    tempPath = Io_ConfigGetPath("Locations", "User_Files", buffer, NULL);
    if (tempPath == NULL) {
        Io_Free(sourcePath);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);

        DebugPrint("UserCreate: Unable to retrieve temporary file location.\n");
        return -1;
    }

    // Copy default file
    if (!CopyFileA(sourcePath, tempPath, FALSE)) {
        error = GetLastError();

        // Free resources
        Io_Free(sourcePath);
        Io_Free(tempPath);

        // Restore system error code
        SetLastError(error);

        DebugPrint("UserCreate: Unable to copy default file (error %lu).\n", GetLastError());
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
BOOL
MODULE_CALL
UserRename(
    char *userName,
    INT32 userId,
    char *newName
    )
{
    DebugPrint("UserRename: userName=\"%s\" userId=%i newName=\"%s\"\n", userName, userId, newName);
    return userModule->RegisterAs(userModule, userName, newName);
}

static
BOOL
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
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        DebugPrint("UserDelete: Unable to retrieve file location.\n");
        return 1;
    }

    // Delete data file and free resources
    DeleteFileA(filePath);
    Io_Free(filePath);

    // Unregister user
    return userModule->Unregister(userModule, userName);
}

static
BOOL
MODULE_CALL
UserLock(
    USERFILE *userFile
    )
{
    DebugPrint("UserLock: userFile=%p\n", userFile);
    return 0;
}

static
BOOL
MODULE_CALL
UserUnlock(
    USERFILE *userFile
    )
{
    DebugPrint("UserUnlock: userFile=%p\n", userFile);
    return 0;
}

static
INT
MODULE_CALL
UserRead(
    char *filePath,
    USERFILE *userFile
    )
{
    char *buffer;
    DWORD bytesRead;
    DWORD error;
    DWORD fileSize;
    INT result;
    USER_CONTEXT *context;

    DebugPrint("UserRead: filePath=\"%s\" userFile=%p\n", filePath, userFile);

    // Allocate user context
    context = (USER_CONTEXT *)Io_Allocate(sizeof(USER_CONTEXT));
    if (context == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        DebugPrint("UserRead: Unable to allocate user context.\n");
        return UM_FATAL;
    }

    buffer = NULL;
    result = UM_FATAL;

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
    userFile->Gid         = userFile->Groups[0];
    userFile->lpInternal  = context;
    result                = UM_SUCCESS;

end:
    // Free objects and resources
    if (result != UM_SUCCESS) {
        error = GetLastError();
        if (context->fileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(context->fileHandle);
        }
        Io_Free(context);
        Io_Free(buffer);

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
    buffer.size = 4096;
    buffer.len  = 0;
    buffer.buf  = (char *)Io_Allocate(buffer.size);

    if (buffer.buf == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        DebugPrint("UserWrite: Unable to allocate write buffer.\n");
        return 1;
    }

    // Dump user data to buffer
    Io_UserFile2Ascii(&buffer, userFile);

    // Write buffer to file
    SetFilePointer(context->fileHandle, 0, 0, FILE_BEGIN);
    if (!WriteFile(context->fileHandle, buffer.buf, buffer.len, &bytesWritten, NULL)) {
        error = GetLastError();
        Io_Free(buffer.buf);
        SetLastError(error);

        DebugPrint("UserWrite: Unable to write file (error %lu).\n", GetLastError());
        return 1;
    }

    // Truncate remaining data
    SetEndOfFile(context->fileHandle);

    Io_Free(buffer.buf);
    return 0;
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
BOOL
MODULE_CALL
UserClose(
    USERFILE *userFile
    )
{
    BOOL result;
    USER_CONTEXT *context;

    DebugPrint("UserClose: userFile=%p\n", userFile);

    // Retrieve user context
    context = (USER_CONTEXT *)userFile->lpInternal;
    if (context == NULL) {
        DebugPrint("UserClose: User context already freed.\n");
        return FALSE;
    }

    // Free objects and resources
    result = CloseHandle(context->fileHandle);
    Io_Free(context);

    return result;
}
