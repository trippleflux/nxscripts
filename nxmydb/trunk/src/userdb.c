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
    TRACE("dbContext=%p userName=%s userFile=%p\n", dbContext, userName, userFile);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserRename(DB_CONTEXT *dbContext, CHAR *userName, CHAR *newName)
{
    ASSERT(dbContext != NULL);
    ASSERT(userName != NULL);
    ASSERT(newName != NULL);
    TRACE("dbContext=%p userName=%s newName=%s\n", dbContext, userName, newName);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserDelete(DB_CONTEXT *dbContext, CHAR *userName)
{
    ASSERT(dbContext != NULL);
    ASSERT(userName != NULL);
    TRACE("dbContext=%p userName=%s\n", dbContext, userName);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserLock(DB_CONTEXT *dbContext, USERFILE *userFile)
{
    ASSERT(dbContext != NULL);
    ASSERT(userFile != NULL);
    TRACE("dbContext=%p userFile=%p\n", dbContext, userFile);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserUnlock(DB_CONTEXT *dbContext, USERFILE *userFile)
{
    ASSERT(dbContext != NULL);
    ASSERT(userFile != NULL);
    TRACE("dbContext=%p userFile=%p\n", dbContext, userFile);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserOpen(DB_CONTEXT *dbContext, CHAR *userName, USERFILE *userFile)
{
    ASSERT(dbContext != NULL);
    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    TRACE("dbContext=%p userName=%s userFile=%p\n", dbContext, userName, userFile);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserWrite(DB_CONTEXT *dbContext, USERFILE *userFile)
{
    ASSERT(dbContext != NULL);
    ASSERT(userFile != NULL);
    TRACE("dbContext=%p userFile=%p\n", dbContext, userFile);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserClose(USERFILE *userFile)
{
    ASSERT(userFile != NULL);
    TRACE("userFile=%p\n", userFile);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserRefresh(DB_CONTEXT *dbContext)
{
    ASSERT(dbContext != NULL);
    TRACE("dbContext=%p\n", dbContext);

    return ERROR_INTERNAL_ERROR;
}
