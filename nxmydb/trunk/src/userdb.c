/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    User Database Backend

Author:
    neoxed (neoxed@gmail.com) Jun 5, 2006

Abstract:
    User database storage backend.

*/

#include "mydb.h"

BOOL
DbUserRefresh(
    DB_CONTEXT *dbContext
    )
{
    ASSERT(dbContext != NULL);
    DebugPrint("DbUserRefresh", "dbContext=%p", dbContext);

    return TRUE;
}

BOOL
DbUserCreate(
    DB_CONTEXT *dbContext,
    char *userName,
    USERFILE *userFile
    )
{
    ASSERT(dbContext != NULL);
    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    DebugPrint("DbUserCreate", "dbContext=%p userName=\"%s\" userFile=%p\n", dbContext, userName, userFile);

    return TRUE;
}

BOOL
DbUserRename(
    DB_CONTEXT *dbContext,
    char *userName,
    char *newName
    )
{
    ASSERT(dbContext != NULL);
    ASSERT(userName != NULL);
    ASSERT(newName != NULL);
    DebugPrint("DbUserRename", "dbContext=%p userName=\"%s\" newName=\"%s\"\n", dbContext, userName, newName);

    return TRUE;
}

BOOL
DbUserDelete(
    DB_CONTEXT *dbContext,
    char *userName
    )
{
    ASSERT(dbContext != NULL);
    ASSERT(userName != NULL);
    DebugPrint("DbUserDelete", "dbContext=%p userName=\"%s\"\n", dbContext, userName);

    return TRUE;
}

BOOL
DbUserLock(
    DB_CONTEXT *dbContext,
    USERFILE *userFile
    )
{
    ASSERT(dbContext != NULL);
    ASSERT(userFile != NULL);
    DebugPrint("DbUserLock", "dbContext=%p userFile=%p", dbContext, userFile);

    return TRUE;
}

BOOL
DbUserUnlock(
    DB_CONTEXT *dbContext,
    USERFILE *userFile
    )
{
    ASSERT(dbContext != NULL);
    ASSERT(userFile != NULL);
    DebugPrint("DbUserUnlock", "dbContext=%p userFile=%p", dbContext, userFile);

    return TRUE;
}

BOOL
DbUserOpen(
    DB_CONTEXT *dbContext,
    char *userName,
    USERFILE *userFile
    )
{
    ASSERT(dbContext != NULL);
    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    DebugPrint("DbUserOpen", "dbContext=%p userName=\"%s\" userFile=%p\n", dbContext, userName, userFile);

    return TRUE;
}

BOOL
DbUserWrite(
    DB_CONTEXT *dbContext,
    USERFILE *userFile
    )
{
    ASSERT(dbContext != NULL);
    ASSERT(userFile != NULL);
    DebugPrint("DbUserWrite", "dbContext=%p userFile=%p", dbContext, userFile);

    return TRUE;
}

BOOL
DbUserClose(
    USER_CONTEXT *userContext
    )
{
    ASSERT(userContext != NULL);
    DebugPrint("DbUserClose", "userContext=%p", userContext);

    // Release reserved database connection
    if (userContext->dbReserved != NULL) {
        DbRelease(userContext->dbReserved);
    }

    return TRUE;
}
