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
// Function declarations
//

static INT   MODULE_CALL UserFinalize(void);
static INT32 MODULE_CALL UserCreate(char *userName);
static INT   MODULE_CALL UserRename(char *userName, INT32 userId, char *newName);
static INT   MODULE_CALL UserDelete(char *userName, INT32 userId);
static INT   MODULE_CALL UserLock(USERFILE *userFile);
static INT   MODULE_CALL UserUnlock(USERFILE *userFile);
static INT   MODULE_CALL UserOpen(char *userName, USERFILE *userFile);
static INT   MODULE_CALL UserWrite(USERFILE *userFile);
static INT   MODULE_CALL UserClose(USERFILE *userFile);

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
    DebugPrint("UserInit", "module=%p\n", module);

    // Initialize module structure
    module->tszModuleName = "NXMYDB";
    module->DeInitialize  = UserFinalize;
    module->Create        = UserCreate;
    module->Rename        = UserRename;
    module->Delete        = UserDelete;
    module->Lock          = UserLock;
    module->Unlock        = UserUnlock;
    module->Open          = UserOpen;
    module->Write         = UserWrite;
    module->Close         = UserClose;

    // Initialize procedure table
    if (!InitProcTable(module->GetProc)) {
        DebugPrint("UserInit", "Unable to initialize procedure table.\n");
        return UM_ERROR;
    }
    Io_Putlog(LOG_ERROR, "nxMyDB user module loaded.\r\n");

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
    DebugPrint("UserFinalize", "module=%p\n", userModule);
    Io_Putlog(LOG_ERROR, "nxMyDB user module unloaded.\r\n");

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
    DWORD error;
    INT32 userId;
    INT_CONTEXT *context;
    USERFILE userFile;

    DebugPrint("UserCreate", "userName=\"%s\"\n", userName);

    // Allocate internal context
    context = Io_Allocate(sizeof(INT_CONTEXT));
    if (context == NULL) {
        DebugPrint("UserCreate", "Unable to allocate internal context.\n");

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return -1;
    }

    // Initialize USERFILE structure
    ZeroMemory(&userFile, sizeof(USERFILE));
    userFile.Groups[0]      = NOGROUP_ID;
    userFile.Groups[1]      = -1;
    userFile.AdminGroups[0] = -1;
    userFile.lpInternal     = context;

    // Register user
    userId = userModule->Register(userModule, userName, &userFile);
    if (userId == -1) {
        DebugPrint("UserCreate", "Unable to register user (error %lu).\n", GetLastError());
        goto error;
    }

    // Create user file and read "Default.User"
    if (!FileUserCreate(userId, &userFile)) {
        DebugPrint("UserCreate", "Unable to create user file (error %lu).\n", GetLastError());
        goto error;
    }

    // Create database record
    if (!DbUserCreate(userName, &userFile)) {
        DebugPrint("UserCreate", "Unable to create database record (error %lu).\n", GetLastError());
        goto error;
    }

    return userId;

error:
    // Preserve system error code
    error = GetLastError();

    if (userId == -1) {
        Io_Free(context);
    } else if (UserDelete(userName, userId) != UM_SUCCESS) {
        DebugPrint("UserCreate", "Unable to delete user (error %lu).\n", GetLastError());
    }

    // Restore system error code
    SetLastError(error);
    return -1;
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
    DebugPrint("UserRename", "userName=\"%s\" userId=%i newName=\"%s\"\n", userName, userId, newName);

    // Rename database record
    if (!DbUserRename(userName, newName)) {
        DebugPrint("UserRename", "Unable to rename database record (error %lu).\n", GetLastError());
    }

    // Register user under the new name
    if (userModule->RegisterAs(userModule, userName, newName) != UM_SUCCESS) {
        DebugPrint("UserRename", "Unable to re-register user (error %lu).\n", GetLastError());
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
    DebugPrint("UserDelete", "userName=\"%s\" userId=%i\n", userName, userId);

    // Delete user file
    if (!FileUserDelete(userId)) {
        DebugPrint("UserDelete", "Unable to delete user file (error %lu).\n", GetLastError());
    }

    // Delete database record
    if (!DbUserDelete(userName)) {
        DebugPrint("UserDelete", "Unable to delete database record (error %lu).\n", GetLastError());
    }

    // Unregister user
    if (userModule->Unregister(userModule, userName) != UM_SUCCESS) {
        DebugPrint("UserDelete", "Unable to unregister user (error %lu).\n", GetLastError());
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
    DebugPrint("UserLock", "userFile=%p\n", userFile);

    // Lock user exclusively
    if (!DbUserLock(userFile)) {
        DebugPrint("UserLock", "Unable to lock database record (error %lu).\n", GetLastError());
        return UM_ERROR;
    }

    return UM_SUCCESS;
}

static
INT
MODULE_CALL
UserUnlock(
    USERFILE *userFile
    )
{
    DebugPrint("UserUnlock", "userFile=%p\n", userFile);

    // Unlock user
    if (!DbUserUnlock(userFile)) {
        DebugPrint("UserUnlock", "Unable to unlock database record (error %lu).\n", GetLastError());
        return UM_ERROR;
    }

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
    DebugPrint("UserOpen", "userName=\"%s\" userFile=%p\n", userName, userFile);

    // Allocate internal context
    userFile->lpInternal = Io_Allocate(sizeof(INT_CONTEXT));
    if (userFile->lpInternal == NULL) {
        DebugPrint("UserOpen", "Unable to allocate internal context.\n");

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return UM_FATAL;
    }

    // Open user file
    if (!FileUserOpen(userFile->Uid, userFile->lpInternal)) {
        DebugPrint("UserOpen", "Unable to open user file (error %lu).\n", GetLastError());
        return (GetLastError() == ERROR_FILE_NOT_FOUND) ? UM_DELETED : UM_FATAL;
    }

    // Read database record
    if (!DbUserOpen(userName, userFile)) {
        DebugPrint("UserOpen", "Unable to open database record (error %lu).\n", GetLastError());
        return UM_FATAL;
    }

    return UM_SUCCESS;
}

static
INT
MODULE_CALL
UserWrite(
    USERFILE *userFile
    )
{
    DebugPrint("UserWrite", "userFile=%p\n", userFile);

    // Update user file
    if (!FileUserWrite(userFile)) {
        DebugPrint("UserWrite", "Unable to write user file (error %lu).\n", GetLastError());
    }

    // Update database record
    if (!DbUserWrite(userFile)) {
        DebugPrint("UserWrite", "Unable to write database record (error %lu).\n", GetLastError());
        return UM_ERROR;
    }

    return UM_SUCCESS;
}

static
INT
MODULE_CALL
UserClose(
    USERFILE *userFile
    )
{
    INT_CONTEXT *context = userFile->lpInternal;

    DebugPrint("UserClose", "userFile=%p\n", userFile);

    // Verify internal context
    if (context == NULL) {
        DebugPrint("UserClose", "Internal context already freed.\n");
        return UM_ERROR;
    }

    // Close user file
    if (!FileUserClose(context)) {
        DebugPrint("UserClose", "Unable to close user file (error %lu).\n", GetLastError());
    }

    // Free database resources
    if (!DbUserClose(context)) {
        DebugPrint("UserClose", "Unable to close database record(error %lu).\n", GetLastError());
    }

    // Free internal resources
    Io_Free(context);
    userFile->lpInternal = NULL;

    return UM_SUCCESS;
}
