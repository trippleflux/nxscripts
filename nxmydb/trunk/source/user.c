/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2009 neoxed

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
static INT32 UserCreate(CHAR *userName, INT32 groupId);
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
    ASSERT(module != NULL);
    TRACE("module=%p", module);

    // Initialize module
    module->tszModuleName = MODULE_NAME;
    module->DeInitialize  = UserFinalize;
    module->Create        = UserCreate;
    module->Rename        = UserRename;
    module->Delete        = UserDelete;
    module->Lock          = UserLock;
    module->Unlock        = UserUnlock;
    module->Open          = UserOpen;
    module->Write         = UserWrite;
    module->Close         = UserClose;

    // Initialize database
    if (!DbInit(module->GetProc)) {
        TRACE("Unable to initialize module.");
        return UM_ERROR;
    }

    userModule = module;
    return UM_SUCCESS;
}

static INT UserFinalize(VOID)
{
    // Finalize database
    DbFinalize();

    userModule = NULL;
    return UM_SUCCESS;
}

static INT32 UserCreate(CHAR *userName, INT32 groupId)
{
    DB_CONTEXT  *db;
    MOD_CONTEXT *mod;
    DWORD       result;
    INT32       userId = -1;
    USERFILE    userFile;

    TRACE("userName=%s groupId=%d", userName, groupId);

    if (!DbAcquire(&db)) {
        return userId;
    }

    // Module context is required for all file operations
    mod = MemAllocate(sizeof(MOD_CONTEXT));
    if (mod == NULL) {
        result = ERROR_NOT_ENOUGH_MEMORY;
        LOG_ERROR("Unable to allocate memory for module context.");

    } else {
        // Initialize MOD_CONTEXT structure
        mod->file = INVALID_HANDLE_VALUE;

        // Initialize USERFILE structure
        ZeroMemory(&userFile, sizeof(USERFILE));
        userFile.Groups[0]      = NOGROUP_ID;
        userFile.Groups[1]      = -1;
        userFile.AdminGroups[0] = -1;
        userFile.CreatorUid     = -1;
        userFile.MaxUploads     = -1;
        userFile.MaxDownloads   = -1;
        userFile.lpInternal     = mod;

        // TODO: Read "Default=Group" file

        // Read "Default.User" file
        result = FileUserDefault(&userFile);
        if (result != ERROR_SUCCESS) {
            LOG_WARN("Unable to read \"Default.User\" file (error %lu).", result);
        }

        // Register user
        result = UserRegister(userName, &userFile, &userId);
        if (result != ERROR_SUCCESS) {
            TRACE("Unable to register user (error %lu).", result);
        } else {

            // Create user file
            result = FileUserCreate(userId, &userFile);
            if (result != ERROR_SUCCESS) {
                LOG_WARN("Unable to create user file (error %lu).", result);
            } else {

                // Create database record
                result = DbUserCreate(db, userName, &userFile);
                if (result != ERROR_SUCCESS) {
                    LOG_WARN("Unable to create database record (error %lu).", result);
                }
            }

            // If the file or database creation failed, clean-up the user file
            if (result != ERROR_SUCCESS) {
                FileUserDelete(userId);
                FileUserClose(&userFile);
                UserUnregister(userName);
            }
        }

        if (result != ERROR_SUCCESS) {
            // Free module context after all file operations
            MemFree(mod);

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
    TRACE("userName=%s userId=%d newName=%s", userName, userId, newName);

    if (!DbAcquire(&db)) {
        return UM_ERROR;
    }

    // Rename database record
    result = DbUserRename(db, userName, newName);
    if (result != ERROR_SUCCESS) {
        LOG_ERROR("Unable to rename user database record from \"%s\" to \"%s\" (error %lu).",
            userName, newName, result);

    } else {
        // Register user under the new name
        result = UserRegisterAs(userName, newName);
    }

    DbRelease(db);

    SetLastError(result);
    return (result == ERROR_SUCCESS) ? UM_SUCCESS : UM_ERROR;
}

static INT UserDelete(CHAR *userName, INT32 userId)
{
    DB_CONTEXT *db;
    DWORD       result;

    TRACE("userName=%s userId=%d", userName, userId);

    if (!DbAcquire(&db)) {
        return UM_ERROR;
    }

    // Delete user file (success does not matter)
    result = FileUserDelete(userId);
    if (result != ERROR_SUCCESS) {
        LOG_ERROR("Unable to delete user file for \"%s\" (error %lu).", userName, result);
    }

    // Delete database record
    result = DbUserDelete(db, userName);
    if (result != ERROR_SUCCESS) {
        LOG_ERROR("Unable to delete user database record for \"%s\" (error %lu).", userName, result);

    } else {
        // Unregister user
        result = UserUnregister(userName);
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

    TRACE("userFile=%p", userFile);

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
            LOG_ERROR("Unable to lock user \"%s\" (error %lu).", userName, result);
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

    TRACE("userFile=%p", userFile);

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
            LOG_ERROR("Unable to unlock user \"%s\" (error %lu).", userName, result);
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

    TRACE("userName=%s userFile=%p", userName, userFile);

    if (!DbAcquire(&db)) {
        // Return UM_DELETED instead of UM_ERROR to work around a bug in ioFTPD.
        return UM_DELETED;
    }

    // Module context is required for all file operations
    mod = MemAllocate(sizeof(MOD_CONTEXT));
    if (mod == NULL) {
        result = ERROR_NOT_ENOUGH_MEMORY;
        LOG_ERROR("Unable to allocate memory for module context.");

    } else {
        // Initialize MOD_CONTEXT structure
        mod->file            = INVALID_HANDLE_VALUE;
        userFile->lpInternal = mod;

        // Open user file
        result = FileUserOpen(userFile->Uid, userFile);
        if (result != ERROR_SUCCESS) {
            LOG_WARN("Unable to open user file for \"%s\" (error %lu).", userName, result);
        } else {

            // Read database record
            result = DbUserOpen(db, userName, userFile);
            if (result != ERROR_SUCCESS) {
                LOG_WARN("Unable to open user database record for \"%s\" (error %lu).", userName, result);

                // Clean-up user file
                FileUserClose(userFile);
            }
        }

        // Free module context if the file/database open failed
        if (result != ERROR_SUCCESS) {
            MemFree(mod);
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

    TRACE("userFile=%p", userFile);

    if (!DbAcquire(&db)) {
        return UM_ERROR;
    }

    // Resolve user ID to user name
    userName = Io_Uid2User(userFile->Uid);
    if (userName == NULL) {
        result = ERROR_ID_NOT_FOUND;

    } else {
        // Update user file (success does not matter)
        result = FileUserWrite(userFile);
        if (result != ERROR_SUCCESS) {
            LOG_WARN("Unable to write user file for \"%s\" (error %lu).", userName, result);
        }

        // Update user database record
        result = DbUserWrite(db, userName, userFile);
        if (result != ERROR_SUCCESS) {
            LOG_WARN("Unable to write user database record for \"%s\" (error %lu).", userName, result);
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

    TRACE("userFile=%p", userFile);

    mod = userFile->lpInternal;

    if (mod != NULL) {
        // Close user file (success does not matter)
        result = FileUserClose(userFile);
        if (result != ERROR_SUCCESS) {
            LOG_WARN("Unable to close user file (error %lu).", result);
        }

        // Close user database record (success does not matter)
        result = DbUserClose(userFile);
        if (result != ERROR_SUCCESS) {
            LOG_WARN("Unable to close user database record (error %lu).", result);
        }

        // Free module context
        MemFree(mod);
        userFile->lpInternal = NULL;
    }

    return UM_SUCCESS;
}


BOOL UserExists(CHAR *userName)
{
    INT32 userId;

    ASSERT(userName != NULL);
    TRACE("userName=%s", userName);

    userId = Io_User2Uid(userName);
    return (userId != -1);
}

DWORD UserRegister(CHAR *userName, USERFILE *userFile, INT32 *userIdPtr)
{
    DWORD   errorCode = ERROR_SUCCESS;
    INT32   userId;

    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    ASSERT(userIdPtr != NULL);
    TRACE("userName=%s userFile=%p userIdPtr=%p", userName, userFile, userIdPtr);

    userId = userModule->Register(userModule, userName, userFile);

    //
    // The "Register" function returns -1 on failure.
    //
    if (userId == -1) {
        errorCode = GetLastError();
        ASSERT(errorCode != ERROR_SUCCESS);

        TRACE("Unable to register user \"%s\" (error %lu).", userName, errorCode);
    }

    *userIdPtr = userId;
    return errorCode;
}

DWORD UserRegisterAs(CHAR *userName, CHAR *newName)
{
    BOOL    result;
    DWORD   errorCode = ERROR_SUCCESS;

    ASSERT(userName != NULL);
    ASSERT(newName != NULL);
    TRACE("userName=%s newName=%s", userName, newName);

    result = userModule->RegisterAs(userModule, userName, newName);

    //
    // Unlike most boolean functions, the "RegisterAs"
    // function returns a non-zero value on failure.
    //
    if (result) {
        errorCode = GetLastError();
        ASSERT(errorCode != ERROR_SUCCESS);

        TRACE("Unable to re-register user \"%s\" as \"%s\" (error %lu).",
            userName, newName, errorCode);
    }

    return errorCode;
}

DWORD UserUnregister(CHAR *userName)
{
    BOOL    result;
    DWORD   errorCode = ERROR_SUCCESS;

    ASSERT(userName != NULL);
    TRACE("userName=%s", userName);

    result = userModule->Unregister(userModule, userName);

    //
    // Unlike most boolean functions, the "Unregister"
    // function returns a non-zero value on failure.
    //
    if (result) {
        errorCode = GetLastError();
        ASSERT(errorCode != ERROR_SUCCESS);

        TRACE("Unable to unregister user \"%s\" (error %lu).", userName, errorCode);
    }

    return errorCode;
}

DWORD UserUpdate(USERFILE *userFile)
{
    BOOL    result;
    DWORD   errorCode = ERROR_SUCCESS;

    ASSERT(userFile != NULL);
    TRACE("userFile=%p", userFile);

    result = userModule->Update(userFile);

    //
    // Unlike most boolean functions, the "Update"
    // function returns a non-zero value on failure.
    //
    if (result) {
        errorCode = GetLastError();
        ASSERT(errorCode != ERROR_SUCCESS);

        TRACE("Unable to update user ID %d (error %lu).", userFile->Uid, errorCode);
    }

    return errorCode;
}

DWORD UserUpdateByName(CHAR *userName, USERFILE *userFile)
{
    BOOL    result;
    DWORD   errorCode = ERROR_SUCCESS;
    INT32   userId;

    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    TRACE("userName=%s userFile=%p", userName, userFile);

    // Resolve the user name to its ID.
    userId = Io_User2Uid(userName);
    if (userId == -1) {
        errorCode = GetLastError();
        ASSERT(errorCode != ERROR_SUCCESS);

        TRACE("Unable to resolve user \"%s\" (error %lu).", userName, errorCode);
    } else {
        // Update the ID member before calling the "Update" function.
        userFile->Uid = userId;

        result = userModule->Update(userFile);

        //
        // Unlike most boolean functions, the "Update"
        // function returns a non-zero value on failure.
        //
        if (result) {
            errorCode = GetLastError();
            ASSERT(errorCode != ERROR_SUCCESS);

            TRACE("Unable to update user \"%s\" (error %lu).", userName, errorCode);
        }
    }

    return errorCode;
}
