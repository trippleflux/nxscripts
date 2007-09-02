/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

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
    // Initialize module structure
    module->tszModuleName = "NXMYDB";
    module->DeInitialize  = GroupFinalize;
    module->Create        = GroupCreate;
    module->Rename        = GroupRename;
    module->Delete        = GroupDelete;
    module->Lock          = GroupLock;
    module->Unlock        = GroupUnlock;
    module->Open          = GroupOpen;
    module->Write         = GroupWrite;
    module->Close         = GroupClose;

    // Initialize module
    if (!DbInit(module->GetProc)) {
        TRACE("Unable to initialize module.\n");
        return GM_ERROR;
    }

    groupModule = module;
    return GM_SUCCESS;
}

static INT GroupFinalize(VOID)
{
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

    TRACE("groupName=%s\n", groupName);

    if (!DbAcquire(&db)) {
        return groupId;
    }

    // Module context is required for all file operations
    mod = Io_Allocate(sizeof(MOD_CONTEXT));
    if (mod == NULL) {
        result = ERROR_NOT_ENOUGH_MEMORY;
        TRACE("Unable to allocate module context.\n");

    } else {
        // Initialize MOD_CONTEXT structure
        mod->file = INVALID_HANDLE_VALUE;

        // Initialize GROUPFILE structure
        ZeroMemory(&groupFile, sizeof(GROUPFILE));
        groupFile.lpInternal = mod;

        // Register group
        groupId = groupModule->Register(groupModule, groupName, &groupFile);
        if (groupId == -1) {
            result = GetLastError();
            TRACE("Unable to register group (error %lu).\n", result);
        } else {

            // Create group file and read "Default.Group"
            result = FileGroupCreate(groupId, &groupFile);
            if (result != ERROR_SUCCESS) {
                TRACE("Unable to create group file (error %lu).\n", result);
            } else {

                // Create database record
                result = DbGroupCreate(db, groupName, &groupFile);
                if (result != ERROR_SUCCESS) {
                    TRACE("Unable to create database record (error %lu).\n", result);
                }
            }

            // If the file or database creation failed, clean-up the group file
            if (result != ERROR_SUCCESS) {
                groupModule->Unregister(groupModule, groupName);
                FileGroupDelete(groupId);
                FileGroupClose(&groupFile);
            }
        }

        if (result != ERROR_SUCCESS) {
            // Free module context after all file operations
            Io_Free(mod);

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

    TRACE("groupName=%s groupId=%d newName=%s\n", groupName, groupId, newName);

    if (!DbAcquire(&db)) {
        return GM_ERROR;
    }

    // Rename database record
    result = DbGroupRename(db, groupName, newName);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to rename group database record (error %lu).\n", result);
    } else {

        // Register group under the new name
        if (groupModule->RegisterAs(groupModule, groupName, newName) != GM_SUCCESS) {
            result = GetLastError();
            TRACE("Unable to re-register group (error %lu).\n", result);
        }
    }

    DbRelease(db);

    SetLastError(result);
    return (result == ERROR_SUCCESS) ? GM_SUCCESS : GM_ERROR;
}

static INT GroupDelete(CHAR *groupName, INT32 groupId)
{
    DB_CONTEXT *db;
    DWORD       result;

    TRACE("groupName=%s groupId=%d\n", groupName, groupId);

    if (!DbAcquire(&db)) {
        return GM_ERROR;
    }

    // Delete group file (success does not matter)
    result = FileGroupDelete(groupId);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to delete group file (error %lu).\n", result);
    }

    // Delete database record
    result = DbGroupDelete(db, groupName);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to delete group database record (error %lu).\n", result);
    } else {

        // Unregister group
        if (groupModule->Unregister(groupModule, groupName) != GM_SUCCESS) {
            result = GetLastError();
            TRACE("Unable to unregister group (error %lu).\n", result);
        }
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

    TRACE("groupFile=%p\n", groupFile);

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
            TRACE("Unable to lock group (error %lu).\n", result);
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

    TRACE("groupFile=%p\n", groupFile);

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
            TRACE("Unable to unlock group (error %lu).\n", result);
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

    TRACE("groupName=%s groupFile=%p\n", groupName, groupFile);

    if (!DbAcquire(&db)) {
        return GM_ERROR;
    }
    // Module context is required for all file operations
    mod = Io_Allocate(sizeof(MOD_CONTEXT));
    if (mod == NULL) {
        result = ERROR_NOT_ENOUGH_MEMORY;
        TRACE("Unable to allocate module context.\n");

    } else {
        // Initialize MOD_CONTEXT structure
        mod->file            = INVALID_HANDLE_VALUE;
        groupFile->lpInternal = mod;

        // Open group file
        result = FileGroupOpen(groupFile->Gid, groupFile);
        if (result != ERROR_SUCCESS) {
            TRACE("Unable to open group file (error %lu).\n", result);
        } else {

            // Read database record
            result = DbGroupOpen(db, groupName, groupFile);
            if (result != ERROR_SUCCESS) {
                TRACE("Unable to open group database record (error %lu).\n", result);

                // Clean-up group file
                FileGroupClose(groupFile);
            }
        }

        // Free module context if the file/database open failed
        if (result != ERROR_SUCCESS) {
            Io_Free(mod);
        }
    }

    DbRelease(db);

    SetLastError(result);
    switch (result) {
        case ERROR_SUCCESS:
            return GM_SUCCESS;

        case ERROR_FILE_NOT_FOUND:
            return GM_DELETED;

        default:
            return GM_ERROR;
    }
}

static INT GroupWrite(GROUPFILE *groupFile)
{
    CHAR       *groupName;
    DB_CONTEXT *db;
    DWORD       result;

    TRACE("groupFile=%p\n", groupFile);

    if (!DbAcquire(&db)) {
        return GM_ERROR;
    }

    // Update group file (success does not matter)
    result = FileGroupWrite(groupFile);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to write group file (error %lu).\n", result);
    }

    // Resolve group ID to group name
    groupName = Io_Gid2Group(groupFile->Gid);
    if (groupName == NULL) {
        result = ERROR_ID_NOT_FOUND;

    } else {
        // Update group database record
        result = DbGroupWrite(db, groupName, groupFile);
        if (result != ERROR_SUCCESS) {
            TRACE("Unable to write group database record (error %lu).\n", result);
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

    TRACE("groupFile=%p\n", groupFile);

    mod = groupFile->lpInternal;

    if (mod != NULL) {
        // Close group file (success does not matter)
        result = FileGroupClose(groupFile);
        if (result != ERROR_SUCCESS) {
            TRACE("Unable to close group file (error %lu).\n", result);
        }

        // Close group database record (success does not matter)
        result = DbGroupClose(groupFile);
        if (result != ERROR_SUCCESS) {
            TRACE("Unable to close group database record (error %lu).\n", result);
        }

        // Free module context
        Io_Free(mod);
        groupFile->lpInternal = NULL;
    }

    return GM_SUCCESS;
}
