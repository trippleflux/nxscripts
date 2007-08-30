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

#include <base.h>
#include <backends.h>
#include <database.h>

DWORD DbUserCreate(DB_CONTEXT *db, CHAR *userName, USERFILE *userFile)
{
    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    TRACE("db=%p userName=%s userFile=%p\n", db, userName, userFile);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserRename(DB_CONTEXT *db, CHAR *userName, CHAR *newName)
{
    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    ASSERT(newName != NULL);
    TRACE("db=%p userName=%s newName=%s\n", db, userName, newName);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserDelete(DB_CONTEXT *db, CHAR *userName)
{
    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    TRACE("db=%p userName=%s\n", db, userName);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserLock(DB_CONTEXT *db, USERFILE *userFile)
{
    ASSERT(db != NULL);
    ASSERT(userFile != NULL);
    TRACE("db=%p userFile=%p\n", db, userFile);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserUnlock(DB_CONTEXT *db, USERFILE *userFile)
{
    ASSERT(db != NULL);
    ASSERT(userFile != NULL);
    TRACE("db=%p userFile=%p\n", db, userFile);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserOpen(DB_CONTEXT *db, CHAR *userName, USERFILE *userFile)
{
    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    TRACE("db=%p userName=%s userFile=%p\n", db, userName, userFile);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserWrite(DB_CONTEXT *db, USERFILE *userFile)
{
    ASSERT(db != NULL);
    ASSERT(userFile != NULL);
    TRACE("db=%p userFile=%p\n", db, userFile);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserClose(USERFILE *userFile)
{
    ASSERT(userFile != NULL);
    TRACE("userFile=%p\n", userFile);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserRefresh(DB_CONTEXT *db)
{
    ASSERT(db != NULL);
    TRACE("db=%p\n", db);

    return ERROR_INTERNAL_ERROR;
}
