/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Group Module

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Entry point and functions for group modules.

*/

#include "mydb.h"

//
// Function declarations
//

static INT   MODULE_CALL GroupFinalize(void);
static INT32 MODULE_CALL GroupCreate(char *groupName);
static INT   MODULE_CALL GroupRename(char *groupName, INT32 groupId, char *newName);
static INT   MODULE_CALL GroupDelete(char *groupName, INT32 groupId);
static INT   MODULE_CALL GroupLock(GROUPFILE *groupFile);
static INT   MODULE_CALL GroupUnlock(GROUPFILE *groupFile);
static INT   MODULE_CALL GroupOpen(char *groupName, GROUPFILE *groupFile);
static INT   MODULE_CALL GroupWrite(GROUPFILE *groupFile);
static INT   MODULE_CALL GroupClose(GROUPFILE *groupFile);

//
// Local variables
//

static GROUP_MODULE *groupModule = NULL;


INT
MODULE_CALL
GroupModuleInit(
    GROUP_MODULE *module
    )
{
    DebugPrint("GroupInit", "module=%p\n", module);

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
        DebugPrint("GroupInit", "Unable to initialize module.\n");
        return GM_ERROR;
    }

    groupModule = module;
    return GM_SUCCESS;
}

static
INT
MODULE_CALL
GroupFinalize(
    void
    )
{
    DebugPrint("GroupFinalize", "module=%p\n", groupModule);
    DbFinalize();

    groupModule = NULL;
    return GM_SUCCESS;
}

static
INT32
MODULE_CALL
GroupCreate(
    char *groupName
    )
{
    DWORD error;
    INT32 groupId;
    GROUP_CONTEXT *context;
    GROUPFILE groupFile;

    DebugPrint("GroupCreate", "groupName=\"%s\"\n", groupName);

    // Allocate group context
    context = Io_Allocate(sizeof(GROUP_CONTEXT));
    if (context == NULL) {
        DebugPrint("GroupCreate", "Unable to allocate group context.\n");

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return -1;
    }

    // Initialize GROUPFILE structure
    ZeroMemory(&groupFile, sizeof(GROUPFILE));
    groupFile.lpInternal = context;

    // Register group
    groupId = groupModule->Register(groupModule, groupName, &groupFile);
    if (groupId == -1) {
        DebugPrint("GroupCreate", "Unable to register group (error %lu).\n", GetLastError());
        goto error;
    }

    // Create group file and read "Default.Group"
    if (!FileGroupCreate(groupId, &groupFile)) {
        DebugPrint("GroupCreate", "Unable to create group file (error %lu).\n", GetLastError());
        goto error;
    }

    // Create database record
    if (!DbGroupCreate(groupName, &groupFile)) {
        DebugPrint("GroupCreate", "Unable to create database record (error %lu).\n", GetLastError());
        goto error;
    }

    return groupId;

error:
    // Preserve system error code
    error = GetLastError();

    if (groupId == -1) {
        Io_Free(context);
    } else if (GroupDelete(groupName, groupId) != GM_SUCCESS) {
        DebugPrint("GroupCreate", "Unable to delete group (error %lu).\n", GetLastError());
    }

    // Restore system error code
    SetLastError(error);
    return -1;
}

static
INT
MODULE_CALL
GroupRename(
    char *groupName,
    INT32 groupId,
    char *newName
    )
{
    DebugPrint("GroupRename", "groupName=\"%s\" groupId=%i newName=\"%s\"\n", groupName, groupId, newName);

    // Rename database record
    if (!DbGroupRename(groupName, newName)) {
        DebugPrint("GroupRename", "Unable to rename database record (error %lu).\n", GetLastError());
    }

    // Register group under the new name
    if (groupModule->RegisterAs(groupModule, groupName, newName) != GM_SUCCESS) {
        DebugPrint("GroupRename", "Unable to re-register group (error %lu).\n", GetLastError());
        return GM_ERROR;
    }

    return GM_SUCCESS;
}

static
INT
MODULE_CALL
GroupDelete(
    char *groupName,
    INT32 groupId
    )
{
    DebugPrint("GroupDelete", "groupName=\"%s\" groupId=%i\n", groupName, groupId);

    // Delete group file
    if (!FileGroupDelete(groupId)) {
        DebugPrint("GroupDelete", "Unable to delete group file (error %lu).\n", GetLastError());
    }

    // Delete database record
    if (!DbGroupDelete(groupName)) {
        DebugPrint("GroupDelete", "Unable to delete database record (error %lu).\n", GetLastError());
    }

    // Unregister group
    if (groupModule->Unregister(groupModule, groupName) != GM_SUCCESS) {
        DebugPrint("GroupDelete", "Unable to unregister group (error %lu).\n", GetLastError());
        return GM_ERROR;
    }

    return GM_SUCCESS;
}

static
INT
MODULE_CALL
GroupLock(
    GROUPFILE *groupFile
    )
{
    DebugPrint("GroupLock", "groupFile=%p\n", groupFile);

    // Lock group exclusively
    if (!DbGroupLock(groupFile)) {
        DebugPrint("GroupLock", "Unable to lock database record (error %lu).\n", GetLastError());
        return GM_ERROR;
    }

    return GM_SUCCESS;
}

static
INT
MODULE_CALL
GroupUnlock(
    GROUPFILE *groupFile
    )
{
    DebugPrint("GroupUnlock", "groupFile=%p\n", groupFile);

    // Unlock group
    if (!DbGroupUnlock(groupFile)) {
        DebugPrint("GroupUnlock", "Unable to unlock database record (error %lu).\n", GetLastError());
        return GM_ERROR;
    }

    return GM_SUCCESS;
}

static
INT
MODULE_CALL
GroupOpen(
    char *groupName,
    GROUPFILE *groupFile
    )
{
    DebugPrint("GroupOpen", "groupName=\"%s\" groupFile=%p\n", groupName, groupFile);

    // Allocate group context
    groupFile->lpInternal = Io_Allocate(sizeof(GROUP_CONTEXT));
    if (groupFile->lpInternal == NULL) {
        DebugPrint("GroupOpen", "Unable to allocate group context.\n");

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return GM_FATAL;
    }

    // Open group file
    if (!FileGroupOpen(groupFile->Gid, groupFile->lpInternal)) {
        DebugPrint("GroupOpen", "Unable to open group file (error %lu).\n", GetLastError());
        return (GetLastError() == ERROR_FILE_NOT_FOUND) ? GM_DELETED : GM_FATAL;
    }

    // Read database record
    if (!DbGroupOpen(groupName, groupFile)) {
        DebugPrint("GroupOpen", "Unable to open database record (error %lu).\n", GetLastError());
        return GM_FATAL;
    }

    return GM_SUCCESS;
}

static
INT
MODULE_CALL
GroupWrite(
    GROUPFILE *groupFile
    )
{
    DebugPrint("GroupWrite", "groupFile=%p\n", groupFile);

    // Update group file
    if (!FileGroupWrite(groupFile)) {
        DebugPrint("GroupWrite", "Unable to write group file (error %lu).\n", GetLastError());
    }

    // Update database record
    if (!DbGroupWrite(groupFile)) {
        DebugPrint("GroupWrite", "Unable to write database record (error %lu).\n", GetLastError());
        return GM_ERROR;
    }

    return GM_SUCCESS;
}

static
INT
MODULE_CALL
GroupClose(
    GROUPFILE *groupFile
    )
{
    GROUP_CONTEXT *context = groupFile->lpInternal;

    DebugPrint("GroupClose", "groupFile=%p\n", groupFile);

    // Verify group context
    if (context == NULL) {
        DebugPrint("GroupClose", "group context already freed.\n");
        return GM_ERROR;
    }

    // Close group file
    if (!FileGroupClose(context)) {
        DebugPrint("GroupClose", "Unable to close group file (error %lu).\n", GetLastError());
    }

    // Free database resources
    if (!DbGroupClose(context)) {
        DebugPrint("GroupClose", "Unable to close database record(error %lu).\n", GetLastError());
    }

    // Free internal resources
    Io_Free(context);
    groupFile->lpInternal = NULL;

    return GM_SUCCESS;
}
