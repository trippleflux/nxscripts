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

static GROUP_MODULE *groupModule = NULL;

static BOOL  MODULE_CALL GroupFinalize(void);
static INT32 MODULE_CALL GroupCreate(char *groupName);
static BOOL  MODULE_CALL GroupRename(char *groupName, INT32 groupId, char *newName);
static BOOL  MODULE_CALL GroupDelete(char *groupName, INT32 groupId);
static BOOL  MODULE_CALL GroupLock(GROUPFILE *groupFile);
static BOOL  MODULE_CALL GroupUnlock(GROUPFILE *groupFile);
static BOOL  MODULE_CALL GroupWrite(GROUPFILE *groupFile);
static INT   MODULE_CALL GroupOpen(char *groupName, GROUPFILE *groupFile);
static BOOL  MODULE_CALL GroupClose(GROUPFILE *groupFile);


BOOL
MODULE_CALL
GroupModuleInit(
    GROUP_MODULE *module
    )
{
    // Initialize module structure.
    module->tszModuleName = "NXMYDB";
    module->DeInitialize  = GroupFinalize;
    module->Create        = GroupCreate;
    module->Delete        = GroupDelete;
    module->Rename        = GroupRename;
    module->Lock          = GroupLock;
    module->Write         = GroupWrite;
    module->Open          = GroupOpen;
    module->Close         = GroupClose;
    module->Unlock        = GroupUnlock;

    // Initialize procedure interface.
    if (!InitProcs(module->GetProc)) {
        return 1;
    }

    groupModule = module;
    return 0;
}

static
BOOL
MODULE_CALL
GroupFinalize(
    void
    )
{
    // Finalize procedure interface.
    FinalizeProcs();

    groupModule = NULL;
    return 0;
}

static
INT32
MODULE_CALL
GroupCreate(
    char *groupName
    )
{
}

static
BOOL
MODULE_CALL
GroupRename(
    char *groupName,
    INT32 groupId,
    char *newName
    )
{
}

static
BOOL
MODULE_CALL
GroupDelete(
    char *groupName,
    INT32 groupId
    )
{
}

static
BOOL
MODULE_CALL
GroupLock(
    GROUPFILE *groupFile
    )
{
}

static
BOOL
MODULE_CALL
GroupUnlock(
    GROUPFILE *groupFile
    )
{
}

static
BOOL
MODULE_CALL
GroupWrite(
    GROUPFILE *groupFile
    )
{
}

static
INT
MODULE_CALL
GroupOpen(
    char *groupName,
    GROUPFILE *groupFile
    )
{
}

static
BOOL
MODULE_CALL
GroupClose(
    GROUPFILE *groupFile
    )
{
}
