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

static GROUP_MODULE *groupModule;

static BOOL  MODULE_CALL GroupFinalize(void);
static INT32 MODULE_CALL GroupCreate(TCHAR *groupName);
static BOOL  MODULE_CALL GroupRename(TCHAR *groupName, INT32 groupId, char *newName);
static BOOL  MODULE_CALL GroupDelete(TCHAR *groupName, INT32 groupId);
static BOOL  MODULE_CALL GroupLock(GROUPFILE *groupFile);
static BOOL  MODULE_CALL GroupUnlock(GROUPFILE *groupFile);
static BOOL  MODULE_CALL GroupWrite(GROUPFILE *groupFile);
static INT   MODULE_CALL GroupOpen(TCHAR *groupName, GROUPFILE *groupFile);
static BOOL  MODULE_CALL GroupClose(GROUPFILE *groupFile);


BOOL
MODULE_CALL
GroupModuleInit(
    GROUP_MODULE *module
    )
{
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
}

static
INT32
MODULE_CALL
GroupCreate(
    TCHAR *groupName
    )
{
}

static
BOOL
MODULE_CALL
GroupRename(
    TCHAR *groupName,
    INT32 groupId,
    char *newName
    )
{
}

static
BOOL
MODULE_CALL
GroupDelete(
    TCHAR *groupName,
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
    TCHAR *groupName,
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
