/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    User Module

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Entry point and functions for the user module.

*/

#include "mydb.h"

static INT   MODULE_CALL UserFinalize(void);
static INT32 MODULE_CALL UserCreate(char *userName);
static INT   MODULE_CALL UserRename(char *userName, INT32 userId, char *newName);
static INT   MODULE_CALL UserDelete(char *userName, INT32 userId);
static INT   MODULE_CALL UserLock(USERFILE *userFile);
static INT   MODULE_CALL UserUnlock(USERFILE *userFile);
static INT   MODULE_CALL UserOpen(char *userName, USERFILE *userFile);
static INT   MODULE_CALL UserWrite(USERFILE *userFile);
static INT   MODULE_CALL UserClose(USERFILE *userFile);

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

    // Initialize module
    if (!DbInit(module->GetProc)) {
        DebugPrint("UserInit", "Unable to initialize module.\n");
        return UM_ERROR;
    }

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
    DbFinalize();

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
    INT32 userId = -1;
    USER_CONTEXT *userContext;
    USERFILE userFile;

    DebugPrint("UserCreate", "userName=\"%s\"\n", userName);

    // Allocate and initialize user context
    userContext = Io_Allocate(sizeof(USER_CONTEXT));
    if (userContext == NULL) {
        DebugPrint("UserCreate", "Unable to allocate user context.\n");

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return -1;
    }
    userContext->fileHandle = INVALID_HANDLE_VALUE;
    userContext->dbReserved = NULL;

    // Acquire a database connection
    if (!DbAcquire(&userContext->dbReserved)) {
        goto failed;
    }

    // Initialize USERFILE structure
    ZeroMemory(&userFile, sizeof(USERFILE));
    userFile.Groups[0]      = NOGROUP_ID;
    userFile.Groups[1]      = -1;
    userFile.AdminGroups[0] = -1;
    userFile.lpInternal     = userContext;

    // Register user
    userId = userModule->Register(userModule, userName, &userFile);
    if (userId == -1) {
        DebugPrint("UserCreate", "Unable to register user (error %lu).\n", GetLastError());
        goto failed;
    }

    // Create user file and read "Default.User"
    if (!FileUserCreate(userId, &userFile)) {
        DebugPrint("UserCreate", "Unable to create user file (error %lu).\n", GetLastError());
        goto failed;
    }

    // Create database record
    if (!DbUserCreate(userContext->dbReserved, userName, &userFile)) {
        DebugPrint("UserCreate", "Unable to create database record (error %lu).\n", GetLastError());
        goto failed;
    }

    DbRelease(userContext->dbReserved);
    userContext->dbReserved = NULL;

    return userId;

failed:
    // Preserve system error code
    error = GetLastError();

    if (userId != -1) {
        // User was not created, just free the context
        DbRelease(userContext->dbReserved);
        Io_Free(userContext);
    } else {
        // TODO: UserDelete() does not work
        Assert(0);

        // Delete created user (will also free the context)
        if (UserDelete(userName, userId) != UM_SUCCESS) {
            DebugPrint("UserCreate", "Unable to delete user (error %lu).\n", GetLastError());
        }
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

    // Without access to the USERFILE structure's lpInternal
    // member, this operation cannot be implemented.
    SetLastError(ERROR_NOT_SUPPORTED);
    return UM_ERROR;

#if 0
    INT result = UM_SUCCESS;
    USER_CONTEXT *userContext = userFile->lpInternal;

    DebugPrint("UserRename", "userName=\"%s\" userId=%i newName=\"%s\"\n", userName, userId, newName);

    // There must be a reserved database connection
    if (userContext->dbReserved == NULL) {
        SetLastError(ERROR_INTERNAL_ERROR);
        return UM_ERROR;
    }

    if (!DbUserRename(userContext->dbReserved, userName, newName)) {
        DebugPrint("UserRename", "Unable to rename database record (error %lu).\n", GetLastError());
    }

    // Register user under the new name
    if (userModule->RegisterAs(userModule, userName, newName) != UM_SUCCESS) {
        DebugPrint("UserRename", "Unable to re-register user (error %lu).\n", GetLastError());
        return UM_ERROR;
    }

    return UM_SUCCESS;
#endif
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

    // Without access to the USERFILE structure's lpInternal
    // member, this operation cannot be implemented.
    SetLastError(ERROR_NOT_SUPPORTED);
    return UM_ERROR;

#if 0
    INT result = UM_SUCCESS;
    USER_CONTEXT *userContext = userFile->lpInternal;

    DebugPrint("UserDelete", "userName=\"%s\" userId=%i\n", userName, userId);

    // There must be a reserved database connection
    if (userContext->dbReserved == NULL) {
        SetLastError(ERROR_INTERNAL_ERROR);
        return UM_ERROR;
    }

    if (!FileUserDelete(userId)) {
        DebugPrint("UserDelete", "Unable to delete user file (error %lu).\n", GetLastError());
    }

    if (!DbUserDelete(userContext->dbReserved, userName)) {
        DebugPrint("UserDelete", "Unable to delete database record (error %lu).\n", GetLastError());
    }

    // Unregister user
    if (userModule->Unregister(userModule, userName) != UM_SUCCESS) {
        DebugPrint("UserDelete", "Unable to unregister user (error %lu).\n", GetLastError());
        return UM_ERROR;
    }

    return result;
#endif
}

static
INT
MODULE_CALL
UserLock(
    USERFILE *userFile
    )
{
    DB_CONTEXT *dbContext;
    USER_CONTEXT *userContext = userFile->lpInternal;

    DebugPrint("UserLock", "userFile=%p\n", userFile);

    // There must not be a reserved database connection
    if (userContext->dbReserved != NULL) {
        SetLastError(ERROR_INTERNAL_ERROR);
        return UM_ERROR;
    }

    if (!DbAcquire(&dbContext)) {
        return UM_ERROR;
    }

    if (!DbUserLock(dbContext, userFile)) {
        DebugPrint("UserLock", "Unable to lock database record (error %lu).\n", GetLastError());
        DbRelease(dbContext);
        return UM_ERROR;
    }

    // Reserve the database connection for future use
    userContext->dbReserved = dbContext;

    return UM_SUCCESS;
}

static
INT
MODULE_CALL
UserUnlock(
    USERFILE *userFile
    )
{
    INT result = UM_SUCCESS;
    USER_CONTEXT *userContext = userFile->lpInternal;

    DebugPrint("UserUnlock", "userFile=%p\n", userFile);

    // There must be a reserved database connection
    if (userContext->dbReserved == NULL) {
        SetLastError(ERROR_INTERNAL_ERROR);
        return UM_ERROR;
    }

    if (!DbUserUnlock(userContext->dbReserved, userFile)) {
        DebugPrint("UserUnlock", "Unable to unlock database record (error %lu).\n", GetLastError());
        result = UM_ERROR;
    }

    // Release reserved database connection
    DbRelease(userContext->dbReserved);
    userContext->dbReserved = NULL;

    return result;
}

static
INT
MODULE_CALL
UserOpen(
    char *userName,
    USERFILE *userFile
    )
{
    DB_CONTEXT *dbContext;
    INT result = UM_SUCCESS;
    USER_CONTEXT *userContext;

    DebugPrint("UserOpen", "userName=\"%s\" userFile=%p\n", userName, userFile);

    // Acquire a database connection
    if (!DbAcquire(&dbContext)) {
        return UM_FATAL;
    }

    // Allocate user context
    userContext = Io_Allocate(sizeof(USER_CONTEXT));
    if (userContext == NULL) {
        DebugPrint("UserOpen", "Unable to allocate user context.\n");

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        result = UM_FATAL;
    } else {
        userContext->fileHandle = INVALID_HANDLE_VALUE;
        userContext->dbReserved = NULL;
        userFile->lpInternal = userContext;

        // Open user file
        if (!FileUserOpen(userFile->Uid, userContext)) {
            DebugPrint("UserOpen", "Unable to open user file (error %lu).\n", GetLastError());
            result = (GetLastError() == ERROR_FILE_NOT_FOUND) ? UM_DELETED : UM_FATAL;
        }

        // Read database record
        if (!DbUserOpen(dbContext, userName, userFile)) {
            DebugPrint("UserOpen", "Unable to open database record (error %lu).\n", GetLastError());
            result = UM_FATAL; // TODO: check deleted
        }

        // Free context if we failed
        if (result != UM_SUCCESS) {
            Io_Free(userContext);
        }
    }

    DbRelease(dbContext);
    return result;
}

static
INT
MODULE_CALL
UserWrite(
    USERFILE *userFile
    )
{
    USER_CONTEXT *userContext = userFile->lpInternal;
    DebugPrint("UserWrite", "userFile=%p\n", userFile);

    // There must be a reserved database connection
    if (userContext->dbReserved == NULL) {
        SetLastError(ERROR_INTERNAL_ERROR);
        return UM_ERROR;
    }

    if (!FileUserWrite(userFile)) {
        DebugPrint("UserWrite", "Unable to write user file (error %lu).\n", GetLastError());
    }

    if (!DbUserWrite(userContext->dbReserved, userFile)) {
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
    DebugPrint("UserClose", "userFile=%p\n", userFile);

    if (userFile->lpInternal == NULL) {
        DebugPrint("UserClose", "User context already freed.\n");
        return UM_ERROR;
    }

    if (!FileUserClose(userFile->lpInternal)) {
        DebugPrint("UserClose", "Unable to close user file (error %lu).\n", GetLastError());
    }

    if (!DbUserClose(userFile->lpInternal)) {
        DebugPrint("UserClose", "Unable to close database record(error %lu).\n", GetLastError());
    }

    // Free user context
    Io_Free(userFile->lpInternal);
    userFile->lpInternal = NULL;

    return UM_SUCCESS;
}
