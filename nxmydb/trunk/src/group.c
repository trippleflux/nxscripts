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

static INT   GroupFinalize(void);
static INT32 GroupCreate(char *groupName);
static INT   GroupRename(char *groupName, INT32 groupId, char *newName);
static INT   GroupDelete(char *groupName, INT32 groupId);
static INT   GroupLock(GROUPFILE *groupFile);
static INT   GroupUnlock(GROUPFILE *groupFile);
static INT   GroupOpen(char *groupName, GROUPFILE *groupFile);
static INT   GroupWrite(GROUPFILE *groupFile);
static INT   GroupClose(GROUPFILE *groupFile);

static GROUP_MODULE *groupModule = NULL;


INT
GroupModuleInit(
    GROUP_MODULE *module
    )
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

static
INT
GroupFinalize(
    void
    )
{
    DbFinalize();

    groupModule = NULL;
    return GM_SUCCESS;
}

static
INT32
GroupCreate(
    char *groupName
    )
{
    DWORD error;
    INT32 groupId;
    GROUP_CONTEXT *context;
    GROUPFILE groupFile;

    // Allocate group context
    context = Io_Allocate(sizeof(GROUP_CONTEXT));
    if (context == NULL) {
        TRACE("Unable to allocate group context.\n");

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return -1;
    }

    // Initialize GROUPFILE structure
    ZeroMemory(&groupFile, sizeof(GROUPFILE));
    groupFile.lpInternal = context;

    // Register group
    groupId = groupModule->Register(groupModule, groupName, &groupFile);
    if (groupId == -1) {
        TRACE("Unable to register group (error %lu).\n", GetLastError());
        goto failed;
    }

    // Create group file and read "Default.Group"
    if (!FileGroupCreate(groupId, &groupFile)) {
        TRACE("Unable to create group file (error %lu).\n", GetLastError());
        goto failed;
    }

    // Create database record
    if (!DbGroupCreate(groupName, &groupFile)) {
        TRACE("Unable to create database record (error %lu).\n", GetLastError());
        goto failed;
    }

    return groupId;

failed:
    // Preserve system error code
    error = GetLastError();

    if (groupId == -1) {
        // Group was not created, just free the context
        Io_Free(context);
    } else {
        // Delete created group (will also free the context)
        if (GroupDelete(groupName, groupId) != GM_SUCCESS) {
            TRACE("Unable to delete group (error %lu).\n", GetLastError());
        }
    }

    // Restore system error code
    SetLastError(error);
    return -1;
}

static
INT
GroupRename(
    char *groupName,
    INT32 groupId,
    char *newName
    )
{
    // Rename database record
    if (!DbGroupRename(groupName, newName)) {
        TRACE("Unable to rename database record (error %lu).\n", GetLastError());
    }

    // Register group under the new name
    if (groupModule->RegisterAs(groupModule, groupName, newName) != GM_SUCCESS) {
        TRACE("Unable to re-register group (error %lu).\n", GetLastError());
        return GM_ERROR;
    }

    return GM_SUCCESS;
}

static
INT
GroupDelete(
    char *groupName,
    INT32 groupId
    )
{
    // Delete group file
    if (!FileGroupDelete(groupId)) {
        TRACE("Unable to delete group file (error %lu).\n", GetLastError());
    }

    // Delete database record
    if (!DbGroupDelete(groupName)) {
        TRACE("Unable to delete database record (error %lu).\n", GetLastError());
    }

    // Unregister group
    if (groupModule->Unregister(groupModule, groupName) != GM_SUCCESS) {
        TRACE("Unable to unregister group (error %lu).\n", GetLastError());
        return GM_ERROR;
    }

    return GM_SUCCESS;
}

static
INT
GroupLock(
    GROUPFILE *groupFile
    )
{
    // Lock group exclusively
    if (!DbGroupLock(groupFile)) {
        TRACE("Unable to lock database record (error %lu).\n", GetLastError());
        return GM_ERROR;
    }

    return GM_SUCCESS;
}

static
INT
GroupUnlock(
    GROUPFILE *groupFile
    )
{
    // Unlock group
    if (!DbGroupUnlock(groupFile)) {
        TRACE("Unable to unlock database record (error %lu).\n", GetLastError());
        return GM_ERROR;
    }

    return GM_SUCCESS;
}

static
INT
GroupOpen(
    char *groupName,
    GROUPFILE *groupFile
    )
{
    // Allocate group context
    groupFile->lpInternal = Io_Allocate(sizeof(GROUP_CONTEXT));
    if (groupFile->lpInternal == NULL) {
        TRACE("Unable to allocate group context.\n");

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return GM_FATAL;
    }

    // Open group file
    if (!FileGroupOpen(groupFile->Gid, groupFile->lpInternal)) {
        TRACE("Unable to open group file (error %lu).\n", GetLastError());
        return (GetLastError() == ERROR_FILE_NOT_FOUND) ? GM_DELETED : GM_FATAL;
    }

    // Read database record
    if (!DbGroupOpen(groupName, groupFile)) {
        TRACE("Unable to open database record (error %lu).\n", GetLastError());
        return GM_FATAL;
    }

    return GM_SUCCESS;
}

static
INT
GroupWrite(
    GROUPFILE *groupFile
    )
{
    // Update group file
    if (!FileGroupWrite(groupFile)) {
        TRACE("Unable to write group file (error %lu).\n", GetLastError());
    }

    // Update database record
    if (!DbGroupWrite(groupFile)) {
        TRACE("Unable to write database record (error %lu).\n", GetLastError());
        return GM_ERROR;
    }

    return GM_SUCCESS;
}

static
INT
GroupClose(
    GROUPFILE *groupFile
    )
{
    if (groupFile->lpInternal == NULL) {
        TRACE("Group context already freed.\n");
        return GM_ERROR;
    }

    // Close group file
    if (!FileGroupClose(groupFile->lpInternal)) {
        TRACE("Unable to close group file (error %lu).\n", GetLastError());
    }

    // Free database resources
    if (!DbGroupClose(groupFile->lpInternal)) {
        TRACE("Unable to close database record(error %lu).\n", GetLastError());
    }

    // Free group context
    Io_Free(groupFile->lpInternal);
    groupFile->lpInternal = NULL;

    return GM_SUCCESS;
}
