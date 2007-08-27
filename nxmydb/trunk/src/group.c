/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Group Module

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Entry point and functions for the group module.

*/

#include "mydb.h"

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
    DB_CONTEXT *dbContext;
    DWORD       result;
    INT32       groupId;
    GROUPFILE    groupFile;

    if (!DbAcquire(&dbContext)) {
        return -1;
    }

    // Initialize GROUPFILE structure
    ZeroMemory(&groupFile, sizeof(GROUPFILE));
    groupFile.lpInternal = INVALID_HANDLE_VALUE;

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
            result = DbGroupCreate(dbContext, groupName, &groupFile);
            if (result != ERROR_SUCCESS) {
                TRACE("Unable to create database record (error %lu).\n", result);

                // Clean-up the file
                FileGroupDelete(groupId);
                FileGroupClose(&groupFile);
            }
        }
    }

    DbRelease(dbContext);

    SetLastError(result);
    return (result == ERROR_SUCCESS) ? groupId : -1;
}

static INT GroupRename(CHAR *groupName, INT32 groupId, CHAR *newName)
{
    DB_CONTEXT *dbContext;
    DWORD       result;

    if (!DbAcquire(&dbContext)) {
        return GM_ERROR;
    }

    // Rename database record
    result = DbGroupRename(dbContext, groupName, newName);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to rename group database record (error %lu).\n", result);
    } else {

        // Register group under the new name
        if (groupModule->RegisterAs(groupModule, groupName, newName) != GM_SUCCESS) {
            result = GetLastError();
            TRACE("Unable to re-register group (error %lu).\n", result);
        }
    }

    DbRelease(dbContext);

    SetLastError(result);
    return (result == ERROR_SUCCESS) ? GM_SUCCESS : GM_ERROR;
}

static INT GroupDelete(CHAR *groupName, INT32 groupId)
{
    DB_CONTEXT *dbContext;
    DWORD       result;

    if (!DbAcquire(&dbContext)) {
        return GM_ERROR;
    }

    // Delete group file (success does not matter)
    result = FileGroupDelete(groupId);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to delete group file (error %lu).\n", result);
    }

    // Delete database record
    result = DbGroupDelete(dbContext, groupName);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to delete group database record (error %lu).\n", result);
    } else {

        // Unregister group
        if (groupModule->Unregister(groupModule, groupName) != GM_SUCCESS) {
            result = GetLastError();
            TRACE("Unable to unregister group (error %lu).\n", result);
        }
    }

    DbRelease(dbContext);

    SetLastError(result);
    return (result == ERROR_SUCCESS) ? GM_SUCCESS : GM_ERROR;
}

static INT GroupLock(GROUPFILE *groupFile)
{
    DB_CONTEXT *dbContext;
    DWORD       result;

    if (!DbAcquire(&dbContext)) {
        return GM_ERROR;
    }

    // Lock group
    result = DbGroupLock(dbContext, groupFile);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to lock group (error %lu).\n", result);
    }

    DbRelease(dbContext);

    SetLastError(result);
    return (result == ERROR_SUCCESS) ? GM_SUCCESS : GM_ERROR;
}

static INT GroupUnlock(GROUPFILE *groupFile)
{
    DB_CONTEXT *dbContext;
    DWORD       result;

    if (!DbAcquire(&dbContext)) {
        return GM_ERROR;
    }

    // Unlock group
    result = DbGroupUnlock(dbContext, groupFile);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to unlock group (error %lu).\n", result);
    }

    DbRelease(dbContext);

    SetLastError(result);
    return (result == ERROR_SUCCESS) ? GM_SUCCESS : GM_ERROR;
}

static INT GroupOpen(CHAR *groupName, GROUPFILE *groupFile)
{
    DB_CONTEXT *dbContext;
    DWORD       result;

    if (!DbAcquire(&dbContext)) {
        return GM_FATAL;
    }

    // Open group file
    result = FileGroupOpen(groupFile->Gid, groupFile);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to open group file (error %lu).\n", result);
    } else {

        // Read database record
        result = DbGroupOpen(dbContext, groupName, groupFile);
        if (result != ERROR_SUCCESS) {
            TRACE("Unable to open group database record (error %lu).\n", result);
        }
    }

    DbRelease(dbContext);

    SetLastError(result);
    switch (result) {
        case ERROR_FILE_NOT_FOUND:
            return GM_DELETED;

        case ERROR_SUCCESS:
            return GM_SUCCESS;

        default:
            return GM_FATAL;
    }
}

static INT GroupWrite(GROUPFILE *groupFile)
{
    DB_CONTEXT *dbContext;
    DWORD       result;

    if (!DbAcquire(&dbContext)) {
        return GM_ERROR;
    }

    // Update group file (success does not matter)
    result = FileGroupWrite(groupFile);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to write group file (error %lu).\n", result);
    }

    // Update group database record
    result = DbGroupWrite(dbContext, groupFile);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to write group database record (error %lu).\n", result);
    }

    DbRelease(dbContext);

    SetLastError(result);
    return (result == ERROR_SUCCESS) ? GM_SUCCESS : GM_ERROR;
}

static INT GroupClose(GROUPFILE *groupFile)
{
    DWORD result;

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

    return GM_SUCCESS;
}
