/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    User Module

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Entry point and functions for the user module.

*/

#include <base.h>
#include <backends.h>
#include <database.h>

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
    DB_CONTEXT  *db;
    MOD_CONTEXT *mod;
    DWORD       result;
    INT32       userId = -1;
    USERFILE    userFile;

    TRACE("userName=%s\n", userName);

    if (!DbAcquire(&db)) {
        return userId;
    }

    // Module context is required for all file operations
    mod = Io_Allocate(sizeof(MOD_CONTEXT));
    if (mod == NULL) {
        result = ERROR_NOT_ENOUGH_MEMORY;
        TRACE("Unable to allocate module context.\n");

    } else {
        // Initialize MOD_CONTEXT structure
        mod->file = INVALID_HANDLE_VALUE;

        // Initialize USERFILE structure
        ZeroMemory(&userFile, sizeof(USERFILE));
        userFile.Groups[0]      = NOGROUP_ID;
        userFile.Groups[1]      = -1;
        userFile.AdminGroups[0] = -1;
        userFile.lpInternal     = mod;

        // Read "Default.User" file
        result = FileUserDefault(&userFile);
        if (result != ERROR_SUCCESS) {
            TRACE("Unable to read \"Default.User\" file (error %lu).\n", result);
        }

        // Register user
        userId = userModule->Register(userModule, userName, &userFile);
        if (userId == -1) {
            result = GetLastError();
            TRACE("Unable to register user (error %lu).\n", result);
        } else {

            // Create user file
            result = FileUserCreate(userId, &userFile);
            if (result != ERROR_SUCCESS) {
                TRACE("Unable to create user file (error %lu).\n", result);
            } else {

                // Create database record
                result = DbUserCreate(db, userName, &userFile);
                if (result != ERROR_SUCCESS) {
                    TRACE("Unable to create database record (error %lu).\n", result);
                }
            }

            // If the file or database creation failed, clean-up the user file
            if (result != ERROR_SUCCESS) {
                userModule->Unregister(userModule, userName);
                FileUserDelete(userId);
                FileUserClose(&userFile);
            }
        }

        if (result != ERROR_SUCCESS) {
            // Free module context after all file operations
            Io_Free(mod);

            // Indicate an error occured by returning an invalid user ID
            userId = -1;
        }
    }

    DbRelease(db);

    SetLastError(result);
    return userId;
}

static INT UserRename(CHAR *userName, INT32 userId, CHAR *newName)
{
    DB_CONTEXT *db;
    DWORD       result;

    UNREFERENCED_PARAMETER(userId);
    TRACE("userName=%s userId=%d newName=%s\n", userName, userId, newName);

    if (!DbAcquire(&db)) {
        return UM_ERROR;
    }

    // Rename database record
    result = DbUserRename(db, userName, newName);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to rename user database record (error %lu).\n", result);
    } else {

        // Register user under the new name
        if (userModule->RegisterAs(userModule, userName, newName) != UM_SUCCESS) {
            result = GetLastError();
            TRACE("Unable to re-register user (error %lu).\n", result);
        }
    }

    DbRelease(db);

    SetLastError(result);
    return (result == ERROR_SUCCESS) ? UM_SUCCESS : UM_ERROR;
}

static INT UserDelete(CHAR *userName, INT32 userId)
{
    DB_CONTEXT *db;
    DWORD       result;

    TRACE("userName=%s userId=%d\n", userName, userId);

    if (!DbAcquire(&db)) {
        return UM_ERROR;
    }

    // Delete user file (success does not matter)
    result = FileUserDelete(userId);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to delete user file (error %lu).\n", result);
    }

    // Delete database record
    result = DbUserDelete(db, userName);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to delete user database record (error %lu).\n", result);
    } else {

        // Unregister user
        if (userModule->Unregister(userModule, userName) != UM_SUCCESS) {
            result = GetLastError();
            TRACE("Unable to unregister user (error %lu).\n", result);
        }
    }

    DbRelease(db);

    SetLastError(result);
    return (result == ERROR_SUCCESS) ? UM_SUCCESS : UM_ERROR;
}

static INT UserLock(USERFILE *userFile)
{
    CHAR       *userName;
    DB_CONTEXT *db;
    DWORD       result;

    TRACE("userFile=%p\n", userFile);

    if (!DbAcquire(&db)) {
        return UM_ERROR;
    }

    // Resolve user ID to user name
    userName = Io_Uid2User(userFile->Uid);
    if (userName == NULL) {
        result = ERROR_ID_NOT_FOUND;

    } else {
        // Lock user
        result = DbUserLock(db, userName, userFile);
        if (result != ERROR_SUCCESS) {
            TRACE("Unable to lock user (error %lu).\n", result);
        }
    }

    DbRelease(db);

    SetLastError(result);
    return (result == ERROR_SUCCESS) ? UM_SUCCESS : UM_ERROR;
}

static INT UserUnlock(USERFILE *userFile)
{
    CHAR       *userName;
    DB_CONTEXT *db;
    DWORD       result;

    TRACE("userFile=%p\n", userFile);

    if (!DbAcquire(&db)) {
        return UM_ERROR;
    }

    // Resolve user ID to user name
    userName = Io_Uid2User(userFile->Uid);
    if (userName == NULL) {
        result = ERROR_ID_NOT_FOUND;

    } else {
        // Unlock user
        result = DbUserUnlock(db, userName);
        if (result != ERROR_SUCCESS) {
            TRACE("Unable to unlock user (error %lu).\n", result);
        }
    }

    DbRelease(db);

    SetLastError(result);
    return (result == ERROR_SUCCESS) ? UM_SUCCESS : UM_ERROR;
}

static INT UserOpen(CHAR *userName, USERFILE *userFile)
{
    DB_CONTEXT *db;
    DWORD       result;
    MOD_CONTEXT *mod;

    TRACE("userName=%s userFile=%p\n", userName, userFile);

    if (!DbAcquire(&db)) {
        return UM_ERROR;
    }
    // Module context is required for all file operations
    mod = Io_Allocate(sizeof(MOD_CONTEXT));
    if (mod == NULL) {
        result = ERROR_NOT_ENOUGH_MEMORY;
        TRACE("Unable to allocate module context.\n");

    } else {
        // Initialize MOD_CONTEXT structure
        mod->file            = INVALID_HANDLE_VALUE;
        userFile->lpInternal = mod;

        // Open user file
        result = FileUserOpen(userFile->Uid, userFile);
        if (result != ERROR_SUCCESS) {
            TRACE("Unable to open user file (error %lu).\n", result);
        } else {

            // Read database record
            result = DbUserOpen(db, userName, userFile);
            if (result != ERROR_SUCCESS) {
                TRACE("Unable to open user database record (error %lu).\n", result);

                // Clean-up user file
                FileUserClose(userFile);
            }
        }

        // Free module context if the file/database open failed
        if (result != ERROR_SUCCESS) {
            Io_Free(mod);
        }
    }

    DbRelease(db);

    //
    // Return UM_DELETED instead of UM_ERROR to work around a bug in ioFTPD. If
    // UM_ERROR is returned, ioFTPD frees part of the USERFILE structure and
    // may crash later on (e.g. if someone issues "SITE USERS").
    //
    SetLastError(result);
    return (result == ERROR_SUCCESS) ? UM_SUCCESS : UM_DELETED;
}

static INT UserWrite(USERFILE *userFile)
{
    CHAR       *userName;
    DB_CONTEXT *db;
    DWORD       result;

    TRACE("userFile=%p\n", userFile);

    if (!DbAcquire(&db)) {
        return UM_ERROR;
    }

    // Update user file (success does not matter)
    result = FileUserWrite(userFile);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to write user file (error %lu).\n", result);
    }

    // Resolve user ID to user name
    userName = Io_Uid2User(userFile->Uid);
    if (userName == NULL) {
        result = ERROR_ID_NOT_FOUND;

    } else {
        // Update user database record
        result = DbUserWrite(db, userName, userFile);
        if (result != ERROR_SUCCESS) {
            TRACE("Unable to write user database record (error %lu).\n", result);
        }
    }

    DbRelease(db);

    SetLastError(result);
    return (result == ERROR_SUCCESS) ? UM_SUCCESS : UM_ERROR;
}

static INT UserClose(USERFILE *userFile)
{
    DWORD       result;
    MOD_CONTEXT *mod;

    TRACE("userFile=%p\n", userFile);

    mod = userFile->lpInternal;

    if (mod != NULL) {
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

        // Free module context
        Io_Free(mod);
        userFile->lpInternal = NULL;
    }

    return UM_SUCCESS;
}
