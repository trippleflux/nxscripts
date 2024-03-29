/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2008 neoxed

Module Name:
    Group Module

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Entry point and functions for the group module.

*/

#include <base.h>
#include <backends.h>
#include <database.h>

static INT   GroupFinalize(VOID);
static INT32 GroupCreate(CHAR *groupName);
static INT   GroupRename(CHAR *groupName, INT32 groupId, CHAR *newName);
static INT   GroupDelete(CHAR *groupName, INT32 groupId);
static INT   GroupLock(GROUPFILE *groupFile);
static INT   GroupUnlock(GROUPFILE *groupFile);
static INT   GroupOpen(CHAR *groupName, GROUPFILE *groupFile);
static INT   GroupWrite(GROUPFILE *groupFile);
static INT   GroupClose(GROUPFILE *groupFile);

static GROUP_MODULE *groupModule = NULL;


INT GroupModuleInit(GROUP_MODULE *module)
{
    ASSERT(module != NULL);
    TRACE("module=%p", module);

    // Initialize module
    module->tszModuleName = MODULE_NAME;
    module->DeInitialize  = GroupFinalize;
    module->Create        = GroupCreate;
    module->Rename        = GroupRename;
    module->Delete        = GroupDelete;
    module->Lock          = GroupLock;
    module->Unlock        = GroupUnlock;
    module->Open          = GroupOpen;
    module->Write         = GroupWrite;
    module->Close         = GroupClose;

    // Initialize database
    if (!DbInit(module->GetProc)) {
        TRACE("Unable to initialize module.");
        return GM_ERROR;
    }

    groupModule = module;
    return GM_SUCCESS;
}

static INT GroupFinalize(VOID)
{
    // Finalize database
    DbFinalize();

    groupModule = NULL;
    return GM_SUCCESS;
}

static INT32 GroupCreate(CHAR *groupName)
{
    DB_CONTEXT  *db;
    MOD_CONTEXT *mod;
    DWORD       result;
    INT32       groupId = -1;
    GROUPFILE    groupFile;

    TRACE("groupName=%s", groupName);

    if (!DbAcquire(&db)) {
        return groupId;
    }

    // Module context is required for all file operations
    mod = MemAllocate(sizeof(MOD_CONTEXT));
    if (mod == NULL) {
        result = ERROR_NOT_ENOUGH_MEMORY;
        LOG_ERROR("Unable to allocate memory for module context.");

    } else {
        // Initialize MOD_CONTEXT structure
        mod->file = INVALID_HANDLE_VALUE;

        // Initialize GROUPFILE structure
        ZeroMemory(&groupFile, sizeof(GROUPFILE));
        groupFile.lpInternal = mod;

        // Read "Default.Group" file
        result = FileGroupDefault(&groupFile);
        if (result != ERROR_SUCCESS) {
            LOG_WARN("Unable to read \"Default.Group\" file (error %lu).", result);
        }

        // Register group
        result = GroupRegister(groupName, &groupFile, &groupId);
        if (result != ERROR_SUCCESS) {
            TRACE("Unable to register group (error %lu).", result);
        } else {

            // Create group file
            result = FileGroupCreate(groupId, &groupFile);
            if (result != ERROR_SUCCESS) {
                LOG_WARN("Unable to create group file (error %lu).", result);
            } else {

                // Create database record
                result = DbGroupCreate(db, groupName, &groupFile);
                if (result != ERROR_SUCCESS) {
                    LOG_WARN("Unable to create database record (error %lu).", result);
                }
            }

            // If the file or database creation failed, clean-up the group file
            if (result != ERROR_SUCCESS) {
                FileGroupDelete(groupId);
                FileGroupClose(&groupFile);
                GroupUnregister(groupName);
            }
        }

        if (result != ERROR_SUCCESS) {
            // Free module context after all file operations
            MemFree(mod);

            // Indicate an error occured by returning an invalid group ID
            groupId = -1;
        }
    }

    DbRelease(db);

    SetLastError(result);
    return groupId;
}

static INT GroupRename(CHAR *groupName, INT32 groupId, CHAR *newName)
{
    DB_CONTEXT *db;
    DWORD       result;

    UNREFERENCED_PARAMETER(groupId);
    TRACE("groupName=%s groupId=%d newName=%s", groupName, groupId, newName);

    if (!DbAcquire(&db)) {
        return GM_ERROR;
    }

    // Rename database record
    result = DbGroupRename(db, groupName, newName);
    if (result != ERROR_SUCCESS) {
        LOG_ERROR("Unable to rename group database record from \"%s\" to \"%s\"  (error %lu).",
            groupName, newName, result);

    } else {
        // Register group under the new name
        result = GroupRegisterAs(groupName, newName);
    }

    DbRelease(db);

    SetLastError(result);
    return (result == ERROR_SUCCESS) ? GM_SUCCESS : GM_ERROR;
}

static INT GroupDelete(CHAR *groupName, INT32 groupId)
{
    DB_CONTEXT *db;
    DWORD       result;

    TRACE("groupName=%s groupId=%d", groupName, groupId);

    if (!DbAcquire(&db)) {
        return GM_ERROR;
    }

    // Delete group file (success does not matter)
    result = FileGroupDelete(groupId);
    if (result != ERROR_SUCCESS) {
        LOG_ERROR("Unable to delete group file for \"%s\" (error %lu).", groupName, result);
    }

    // Delete database record
    result = DbGroupDelete(db, groupName);
    if (result != ERROR_SUCCESS) {
        LOG_ERROR("Unable to delete group database record for \"%s\" (error %lu).", groupName, result);

    } else {
        // Unregister group
        result = GroupUnregister(groupName);
    }

    DbRelease(db);

    SetLastError(result);
    return (result == ERROR_SUCCESS) ? GM_SUCCESS : GM_ERROR;
}

static INT GroupLock(GROUPFILE *groupFile)
{
    CHAR       *groupName;
    DB_CONTEXT *db;
    DWORD       result;

    TRACE("groupFile=%p", groupFile);

    if (!DbAcquire(&db)) {
        return GM_ERROR;
    }

    // Resolve group ID to group name
    groupName = Io_Gid2Group(groupFile->Gid);
    if (groupName == NULL) {
        result = ERROR_ID_NOT_FOUND;

    } else {
        // Lock group
        result = DbGroupLock(db, groupName, groupFile);
        if (result != ERROR_SUCCESS) {
            LOG_ERROR("Unable to lock group \"%s\" (error %lu).", groupName, result);
        }
    }

    DbRelease(db);

    SetLastError(result);
    return (result == ERROR_SUCCESS) ? GM_SUCCESS : GM_ERROR;
}

static INT GroupUnlock(GROUPFILE *groupFile)
{
    CHAR       *groupName;
    DB_CONTEXT *db;
    DWORD       result;

    TRACE("groupFile=%p", groupFile);

    if (!DbAcquire(&db)) {
        return GM_ERROR;
    }

    // Resolve group ID to group name
    groupName = Io_Gid2Group(groupFile->Gid);
    if (groupName == NULL) {
        result = ERROR_ID_NOT_FOUND;

    } else {
        // Unlock group
        result = DbGroupUnlock(db, groupName);
        if (result != ERROR_SUCCESS) {
            LOG_ERROR("Unable to unlock group \"%s\" (error %lu).", groupName, result);
        }
    }

    DbRelease(db);

    SetLastError(result);
    return (result == ERROR_SUCCESS) ? GM_SUCCESS : GM_ERROR;
}

static INT GroupOpen(CHAR *groupName, GROUPFILE *groupFile)
{
    DB_CONTEXT *db;
    DWORD       result;
    MOD_CONTEXT *mod;

    TRACE("groupName=%s groupFile=%p", groupName, groupFile);

    if (!DbAcquire(&db)) {
        // Return GM_DELETED instead of GM_ERROR to work around a bug in ioFTPD.
        return GM_DELETED;
    }

    // Module context is required for all file operations
    mod = MemAllocate(sizeof(MOD_CONTEXT));
    if (mod == NULL) {
        result = ERROR_NOT_ENOUGH_MEMORY;
        LOG_ERROR("Unable to allocate memory for module context.");

    } else {
        // Initialize MOD_CONTEXT structure
        mod->file            = INVALID_HANDLE_VALUE;
        groupFile->lpInternal = mod;

        // Open group file
        result = FileGroupOpen(groupFile->Gid, groupFile);
        if (result != ERROR_SUCCESS) {
            LOG_WARN("Unable to open group file for \"%s\" (error %lu).", groupName, result);
        } else {

            // Read database record
            result = DbGroupOpen(db, groupName, groupFile);
            if (result != ERROR_SUCCESS) {
                LOG_WARN("Unable to open group database record for \"%s\" (error %lu).", groupName, result);

                // Clean-up group file
                FileGroupClose(groupFile);
            }
        }

        // Free module context if the file/database open failed
        if (result != ERROR_SUCCESS) {
            MemFree(mod);
        }
    }

    DbRelease(db);

    //
    // Return GM_DELETED instead of GM_ERROR to work around a bug in ioFTPD. If
    // GM_ERROR is returned, ioFTPD frees part of the GROUPFILE structure and
    // may crash later on (e.g. if someone issues "SITE GROUPS").
    //
    SetLastError(result);
    return (result == ERROR_SUCCESS) ? GM_SUCCESS : GM_DELETED;
}

static INT GroupWrite(GROUPFILE *groupFile)
{
    CHAR       *groupName;
    DB_CONTEXT *db;
    DWORD       result;

    TRACE("groupFile=%p", groupFile);

    if (!DbAcquire(&db)) {
        return GM_ERROR;
    }

    // Resolve group ID to group name
    groupName = Io_Gid2Group(groupFile->Gid);
    if (groupName == NULL) {
        result = ERROR_ID_NOT_FOUND;

    } else {
        // Update group file (success does not matter)
        result = FileGroupWrite(groupFile);
        if (result != ERROR_SUCCESS) {
            LOG_WARN("Unable to write group file for \"%s\" (error %lu).", groupName, result);
        }

        // Update group database record
        result = DbGroupWrite(db, groupName, groupFile);
        if (result != ERROR_SUCCESS) {
            LOG_WARN("Unable to write group database record for \"%s\" (error %lu).", groupName, result);
        }
    }

    DbRelease(db);

    SetLastError(result);
    return (result == ERROR_SUCCESS) ? GM_SUCCESS : GM_ERROR;
}

static INT GroupClose(GROUPFILE *groupFile)
{
    DWORD       result;
    MOD_CONTEXT *mod;

    TRACE("groupFile=%p", groupFile);

    mod = groupFile->lpInternal;

    if (mod != NULL) {
        // Close group file (success does not matter)
        result = FileGroupClose(groupFile);
        if (result != ERROR_SUCCESS) {
            LOG_WARN("Unable to close group file (error %lu).", result);
        }

        // Close group database record (success does not matter)
        result = DbGroupClose(groupFile);
        if (result != ERROR_SUCCESS) {
            LOG_WARN("Unable to close group database record (error %lu).", result);
        }

        // Free module context
        MemFree(mod);
        groupFile->lpInternal = NULL;
    }

    return GM_SUCCESS;
}


BOOL GroupExists(CHAR *groupName)
{
    INT32 groupId;

    ASSERT(groupName != NULL);
    TRACE("groupName=%s", groupName);

    groupId = Io_Group2Gid(groupName);
    return (groupId != -1);
}

DWORD GroupRegister(CHAR *groupName, GROUPFILE *groupFile, INT32 *groupIdPtr)
{
    DWORD   errorCode = ERROR_SUCCESS;
    INT32   groupId;

    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);
    ASSERT(groupIdPtr != NULL);
    TRACE("groupName=%s groupFile=%p groupIdPtr=%p", groupName, groupFile, groupIdPtr);

    groupId = groupModule->Register(groupModule, groupName, groupFile);

    //
    // The "Register" function returns -1 on failure.
    //
    if (groupId == -1) {
        errorCode = GetLastError();
        ASSERT(errorCode != ERROR_SUCCESS);

        TRACE("Unable to register group \"%s\" (error %lu).", groupName, errorCode);
    }

    *groupIdPtr = groupId;
    return errorCode;
}

DWORD GroupRegisterAs(CHAR *groupName, CHAR *newName)
{
    BOOL    result;
    DWORD   errorCode = ERROR_SUCCESS;

    ASSERT(groupName != NULL);
    ASSERT(newName != NULL);
    TRACE("groupName=%s newName=%s", groupName, newName);

    result = groupModule->RegisterAs(groupModule, groupName, newName);

    //
    // Unlike most boolean functions, the "RegisterAs"
    // function returns a non-zero value on failure.
    //
    if (result) {
        errorCode = GetLastError();
        ASSERT(errorCode != ERROR_SUCCESS);

        TRACE("Unable to re-register group \"%s\" as \"%s\" (error %lu).",
            groupName, newName, errorCode);
    }

    return errorCode;
}

DWORD GroupUnregister(CHAR *groupName)
{
    BOOL    result;
    DWORD   errorCode = ERROR_SUCCESS;

    ASSERT(groupName != NULL);
    TRACE("groupName=%s", groupName);

    result = groupModule->Unregister(groupModule, groupName);

    //
    // Unlike most boolean functions, the "Unregister"
    // function returns a non-zero value on failure.
    //
    if (result) {
        errorCode = GetLastError();
        ASSERT(errorCode != ERROR_SUCCESS);

        TRACE("Unable to unregister group \"%s\" (error %lu).", groupName, errorCode);
    }

    return errorCode;
}

DWORD GroupUpdate(GROUPFILE *groupFile)
{
    BOOL    result;
    DWORD   errorCode = ERROR_SUCCESS;

    ASSERT(groupFile != NULL);
    TRACE("groupFile=%p", groupFile);

    result = groupModule->Update(groupFile);

    //
    // Unlike most boolean functions, the "Update"
    // function returns a non-zero value on failure.
    //
    if (result) {
        errorCode = GetLastError();
        ASSERT(errorCode != ERROR_SUCCESS);

        TRACE("Unable to update group ID %d (error %lu).", groupFile->Gid, errorCode);
    }

    return errorCode;
}

DWORD GroupUpdateByName(CHAR *groupName, GROUPFILE *groupFile)
{
    BOOL    result;
    DWORD   errorCode = ERROR_SUCCESS;
    INT32   groupId;

    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);
    TRACE("groupName=%s groupFile=%p", groupName, groupFile);

    // Resolve the group name to its ID.
    groupId = Io_Group2Gid(groupName);
    if (groupId == -1) {
        errorCode = GetLastError();
        ASSERT(errorCode != ERROR_SUCCESS);

        TRACE("Unable to resolve group \"%s\" (error %lu).", groupName, errorCode);
    } else {
        // Update the ID member before calling the "Update" function.
        groupFile->Gid = groupId;

        result = groupModule->Update(groupFile);

        //
        // Unlike most boolean functions, the "Update"
        // function returns a non-zero value on failure.
        //
        if (result) {
            errorCode = GetLastError();
            ASSERT(errorCode != ERROR_SUCCESS);

            TRACE("Unable to update group \"%s\" (error %lu).", groupName, errorCode);
        }
    }

    return errorCode;
}
