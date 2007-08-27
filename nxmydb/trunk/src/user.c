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

static INT   UserFinalize(VOID);
static INT32 UserCreate(CHAR *userName);
static INT   UserRename(CHAR *userName, INT32 userId, CHAR *newName);
static INT   UserDelete(CHAR *userName, INT32 userId);
static INT   UserLock(USERFILE *userFile);
static INT   UserUnlock(USERFILE *userFile);
static INT   UserOpen(CHAR *userName, USERFILE *userFile);
static INT   UserWrite(USERFILE *userFile);
static INT   UserClose(USERFILE *userFile);

static USER_MODULE *userModule = NULL;


INT UserModuleInit(USER_MODULE *module)
{
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
        TRACE("Unable to initialize module.\n");
        return UM_ERROR;
    }

    userModule = module;
    return UM_SUCCESS;
}

static INT UserFinalize(VOID)
{
    DbFinalize();

    userModule = NULL;
    return UM_SUCCESS;
}

static INT32 UserCreate(CHAR *userName)
{
    DB_CONTEXT *dbContext;
    DWORD       result;
    INT32       userId;
    USERFILE    userFile;

    if (!DbAcquire(&dbContext)) {
        return -1;
    }

    // Initialize USERFILE structure
    ZeroMemory(&userFile, sizeof(USERFILE));
    userFile.Groups[0]      = NOGROUP_ID;
    userFile.Groups[1]      = -1;
    userFile.AdminGroups[0] = -1;
    userFile.lpInternal     = INVALID_HANDLE_VALUE;

    // Register user
    userId = userModule->Register(userModule, userName, &userFile);
    if (userId == -1) {
        result = GetLastError();
        TRACE("Unable to register user (error %lu).\n", result);
    } else {

        // Create user file and read "Default.User"
        result = FileUserCreate(userId, &userFile);
        if (result != ERROR_SUCCESS) {
            TRACE("Unable to create user file (error %lu).\n", result);
        } else {

            // Create database record
            result = DbUserCreate(dbContext, userName, &userFile);
            if (result != ERROR_SUCCESS) {
                TRACE("Unable to create database record (error %lu).\n", result);

                // Clean-up the file
                FileUserDelete(userId);
                FileUserClose(&userFile);
            }
        }
    }

    DbRelease(dbContext);

    SetLastError(result);
    return (result == ERROR_SUCCESS) ? userId : -1;
}

static INT UserRename(CHAR *userName, INT32 userId, CHAR *newName)
{
    DB_CONTEXT *dbContext;
    DWORD       result;

    if (!DbAcquire(&dbContext)) {
        return UM_ERROR;
    }

    // Rename database record
    result = DbUserRename(dbContext, userName, newName);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to rename user database record (error %lu).\n", result);
    } else {

        // Register user under the new name
        if (userModule->RegisterAs(userModule, userName, newName) != UM_SUCCESS) {
            result = GetLastError();
            TRACE("Unable to re-register user (error %lu).\n", result);
        }
    }

    DbRelease(dbContext);

    SetLastError(result);
    return (result == ERROR_SUCCESS) ? UM_SUCCESS : UM_ERROR;
}

static INT UserDelete(CHAR *userName, INT32 userId)
{
    DB_CONTEXT *dbContext;
    DWORD       result;

    if (!DbAcquire(&dbContext)) {
        return UM_ERROR;
    }

    // Delete user file (success does not matter)
    result = FileUserDelete(userId);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to delete user file (error %lu).\n", result);
    }

    // Delete database record
    result = DbUserDelete(dbContext, userName);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to delete user database record (error %lu).\n", result);
    } else {

        // Unregister user
        if (userModule->Unregister(userModule, userName) != UM_SUCCESS) {
            result = GetLastError();
            TRACE("Unable to unregister user (error %lu).\n", result);
        }
    }

    DbRelease(dbContext);

    SetLastError(result);
    return (result == ERROR_SUCCESS) ? UM_SUCCESS : UM_ERROR;
}

static INT UserLock(USERFILE *userFile)
{
    DB_CONTEXT *dbContext;
    DWORD       result;

    if (!DbAcquire(&dbContext)) {
        return UM_ERROR;
    }

    // Lock user
    result = DbUserLock(dbContext, userFile);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to lock user (error %lu).\n", result);
    }

    DbRelease(dbContext);

    SetLastError(result);
    return (result == ERROR_SUCCESS) ? UM_SUCCESS : UM_ERROR;
}

static INT UserUnlock(USERFILE *userFile)
{
    DB_CONTEXT *dbContext;
    DWORD       result;

    if (!DbAcquire(&dbContext)) {
        return UM_ERROR;
    }

    // Unlock user
    result = DbUserUnlock(dbContext, userFile);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to unlock user (error %lu).\n", result);
    }

    DbRelease(dbContext);

    SetLastError(result);
    return (result == ERROR_SUCCESS) ? UM_SUCCESS : UM_ERROR;
}

static INT UserOpen(CHAR *userName, USERFILE *userFile)
{
    DB_CONTEXT *dbContext;
    DWORD       result;

    if (!DbAcquire(&dbContext)) {
        return UM_FATAL;
    }

    // Open user file
    result = FileUserOpen(userFile->Uid, userFile);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to open user file (error %lu).\n", result);
    } else {

        // Read database record
        result = DbUserOpen(dbContext, userName, userFile);
        if (result != ERROR_SUCCESS) {
            TRACE("Unable to open user database record (error %lu).\n", result);
        }
    }

    DbRelease(dbContext);

    SetLastError(result);
    switch (result) {
        case ERROR_FILE_NOT_FOUND:
            return UM_DELETED;

        case ERROR_SUCCESS:
            return UM_SUCCESS;

        default:
            return UM_FATAL;
    }
}

static INT UserWrite(USERFILE *userFile)
{
    DB_CONTEXT *dbContext;
    DWORD       result;

    if (!DbAcquire(&dbContext)) {
        return UM_ERROR;
    }

    // Update user file (success does not matter)
    result = FileUserWrite(userFile);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to write user file (error %lu).\n", result);
    }

    // Update user database record
    result = DbUserWrite(dbContext, userFile);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to write user database record (error %lu).\n", result);
    }

    DbRelease(dbContext);

    SetLastError(result);
    return (result == ERROR_SUCCESS) ? UM_SUCCESS : UM_ERROR;
}

static INT UserClose(USERFILE *userFile)
{
    DWORD result;

    // Close user file (success does not matter)
    result = FileUserClose(userFile);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to close user file (error %lu).\n", result);
    }

    // Close user database record (success does not matter)
    result = DbUserClose(userFile);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to close user database record (error %lu).\n", result);
    }

    return UM_SUCCESS;
}
