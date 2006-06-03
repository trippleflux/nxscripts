/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    User Module

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Entry point and functions for user modules.

*/

#include "mydb.h"

static USER_MODULE *userModule = NULL;

static BOOL  MODULE_CALL UserFinalize(void);
static INT32 MODULE_CALL UserCreate(char *userName);
static BOOL  MODULE_CALL UserRename(char *userName, INT32 userId, char *newName);
static BOOL  MODULE_CALL UserDelete(char *userName, INT32 userId);
static BOOL  MODULE_CALL UserLock(USERFILE *userFile);
static BOOL  MODULE_CALL UserUnlock(USERFILE *userFile);
static BOOL  MODULE_CALL UserWrite(USERFILE *userFile);
static INT   MODULE_CALL UserOpen(char *userName, USERFILE *userFile);
static BOOL  MODULE_CALL UserClose(USERFILE *userFile);


BOOL
MODULE_CALL
UserModuleInit(
    USER_MODULE *module
    )
{
    // Initialize module structure.
    module->tszModuleName = "NXMYDB";
    module->DeInitialize  = UserFinalize;
    module->Create        = UserCreate;
    module->Delete        = UserDelete;
    module->Rename        = UserRename;
    module->Lock          = UserLock;
    module->Write         = UserWrite;
    module->Open          = UserOpen;
    module->Close         = UserClose;
    module->Unlock        = UserUnlock;

    // Initialize procedure interface.
    if (!InitProcs(module->GetProc)) {
        return 1;
    }

    userModule = module;
    return 0;
}

static
BOOL
MODULE_CALL
UserFinalize(
    void
    )
{
    // Finalize procedure interface.
    FinalizeProcs();

    userModule = NULL;
    return 0;
}

static
INT32
MODULE_CALL
UserCreate(
    char *userName
    )
{
}

static
BOOL
MODULE_CALL
UserRename(
    char *userName,
    INT32 userId,
    char *newName
    )
{
}

static
BOOL
MODULE_CALL
UserDelete(
    char *userName,
    INT32 userId
    )
{
}

static
BOOL
MODULE_CALL
UserLock(
    USERFILE *userFile
    )
{
}

static
BOOL
MODULE_CALL
UserUnlock(
    USERFILE *userFile
    )
{
}

static
BOOL
MODULE_CALL
UserWrite(
    USERFILE *userFile
    )
{
}

static
INT
MODULE_CALL
UserOpen(
    char *userName,
    USERFILE *userFile
    )
{
}

static
BOOL
MODULE_CALL
UserClose(
    USERFILE *userFile
    )
{
}
