/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    User Database Backend

Author:
    neoxed (neoxed@gmail.com) Jun 5, 2006

Abstract:
    User database storage backend.

*/

#include "mydb.h"

DWORD DbUserCreate(DB_CONTEXT *dbContext, CHAR *userName, USERFILE *userFile)
{
    ASSERT(dbContext != NULL);
    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserRename(DB_CONTEXT *dbContext, CHAR *userName, CHAR *newName)
{
    ASSERT(dbContext != NULL);
    ASSERT(userName != NULL);
    ASSERT(newName != NULL);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserDelete(DB_CONTEXT *dbContext, CHAR *userName)
{
    ASSERT(dbContext != NULL);
    ASSERT(userName != NULL);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserLock(DB_CONTEXT *dbContext, USERFILE *userFile)
{
    ASSERT(dbContext != NULL);
    ASSERT(userFile != NULL);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserUnlock(DB_CONTEXT *dbContext, USERFILE *userFile)
{
    ASSERT(dbContext != NULL);
    ASSERT(userFile != NULL);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserOpen(DB_CONTEXT *dbContext, CHAR *userName, USERFILE *userFile)
{
    ASSERT(dbContext != NULL);
    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserWrite(DB_CONTEXT *dbContext, USERFILE *userFile)
{
    ASSERT(dbContext != NULL);
    ASSERT(userFile != NULL);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserClose(USERFILE *userFile)
{
    ASSERT(userFile != NULL);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserRefresh(DB_CONTEXT *dbContext)
{
    ASSERT(dbContext != NULL);

    return ERROR_INTERNAL_ERROR;
}
