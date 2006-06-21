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
    DB_CONTEXT *dbContext;
    DWORD error;
    INT32 userId = -1;
    USER_CONTEXT *userContext;
    USERFILE userFile;

    DebugPrint("UserCreate", "userName=\"%s\"\n", userName);

    // Acquire a database connection
    if (!DbAcquire(&dbContext)) {
        return -1;
    }

    // Allocate user context
    userContext = Io_Allocate(sizeof(USER_CONTEXT));
    if (userContext == NULL) {
        DebugPrint("UserCreate", "Unable to allocate user context.\n");
        DbRelease(dbContext);

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return -1;
    }
    userContext->fileHandle = INVALID_HANDLE_VALUE;
    userContext->db   = NULL;

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
    if (!DbUserCreate(dbContext, userName, &userFile)) {
        DebugPrint("UserCreate", "Unable to create database record (error %lu).\n", GetLastError());
        goto failed;
    }

    DbRelease(dbContext);
    return userId;

failed:
    // Preserve system error code
    error = GetLastError();

    if (userId != -1) {
        // User was not created, just free the context
        Io_Free(userContext);
    } else {
        // Delete created user (will also free the context)
        if (UserDelete(userName, userId) != UM_SUCCESS) {
            DebugPrint("UserCreate", "Unable to delete user (error %lu).\n", GetLastError());
        }
    }

    DbRelease(dbContext);

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

    // No USERFILE structure == No dice
    return UM_ERROR;

#if 0
    INT result = UM_SUCCESS;
    USER_CONTEXT *userContext = userFile->lpInternal;

    DebugPrint("UserRename", "userName=\"%s\" userId=%i newName=\"%s\"\n", userName, userId, newName);

    // There must be a reserved database connection
    if (userContext->db == NULL) {
        SetLastError(ERROR_INTERNAL_ERROR);
        return UM_ERROR;
    }

    if (!DbUserRename(userContext->db, userName, newName)) {
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

    // No USERFILE structure == No dice
    return UM_ERROR;

#if 0
    INT result = UM_SUCCESS;
    USER_CONTEXT *userContext = userFile->lpInternal;

    DebugPrint("UserDelete", "userName=\"%s\" userId=%i\n", userName, userId);

    // There must be a reserved database connection
    if (userContext->db == NULL) {
        SetLastError(ERROR_INTERNAL_ERROR);
        return UM_ERROR;
    }

    if (!FileUserDelete(userId)) {
        DebugPrint("UserDelete", "Unable to delete user file (error %lu).\n", GetLastError());
    }

    if (!DbUserDelete(userContext->db, userName)) {
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
    if (userContext->db != NULL) {
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

    // Reserve the database connection for the following operations
    userContext->db = dbContext;
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
    if (userContext->db == NULL) {
        SetLastError(ERROR_INTERNAL_ERROR);
        return UM_ERROR;
    }

    if (!DbUserUnlock(userContext->db, userFile)) {
        DebugPrint("UserUnlock", "Unable to unlock database record (error %lu).\n", GetLastError());
        result = UM_ERROR;
    }

    DbRelease(userContext->db);
    userContext->db = NULL;
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

    DebugPrint("UserOpen", "userName=\"%s\" userFile=%p\n", userName, userFile);

    // Acquire a database connection
    if (!DbAcquire(&dbContext)) {
        return UM_FATAL;
    }

    // Allocate user context
    userFile->lpInternal = Io_Allocate(sizeof(USER_CONTEXT));
    if (userFile->lpInternal == NULL) {
        DebugPrint("UserOpen", "Unable to allocate user context.\n");

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        result = UM_FATAL;
    } else {
        // Open user file
        if (!FileUserOpen(userFile->Uid, userFile->lpInternal)) {
            DebugPrint("UserOpen", "Unable to open user file (error %lu).\n", GetLastError());
            result = (GetLastError() == ERROR_FILE_NOT_FOUND) ? UM_DELETED : UM_FATAL;
        }

        // Read database record
        if (!DbUserOpen(dbContext, userName, userFile)) {
            DebugPrint("UserOpen", "Unable to open database record (error %lu).\n", GetLastError());
            result = UM_FATAL; // TODO: check deleted
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
    if (userContext->db == NULL) {
        SetLastError(ERROR_INTERNAL_ERROR);
        return UM_ERROR;
    }

    if (!FileUserWrite(userFile)) {
        DebugPrint("UserWrite", "Unable to write user file (error %lu).\n", GetLastError());
    }

    if (!DbUserWrite(userContext->db, userFile)) {
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
