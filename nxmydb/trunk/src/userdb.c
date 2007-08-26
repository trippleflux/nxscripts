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

    return TRUE;
}

BOOL
DbUserClose(
    USER_CONTEXT *userContext
    )
{
    ASSERT(userContext != NULL);

    // Release reserved database connection
    if (userContext->dbReserved != NULL) {
        DbRelease(userContext->dbReserved);
    }

    return TRUE;
}
