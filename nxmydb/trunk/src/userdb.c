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
DbUserCreate(
    char *userName,
    USERFILE *userFile
    )
{
    DB_CONTEXT *context;

    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    DebugPrint("DbUserCreate", "userName=\"%s\" userFile=%p\n", userName, userFile);

    if (!DbAcquire(&context)) {
        return FALSE;
    }

    DbRelease(context);
    return TRUE;
}

BOOL
DbUserRename(
    char *userName,
    char *newName
    )
{
    DB_CONTEXT *context;

    ASSERT(userName != NULL);
    ASSERT(newName != NULL);
    DebugPrint("DbUserRename", "userName=\"%s\" newName=\"%s\"\n", userName, newName);

    if (!DbAcquire(&context)) {
        return FALSE;
    }

    DbRelease(context);
    return TRUE;
}

BOOL
DbUserDelete(
    char *userName
    )
{
    DB_CONTEXT *context;

    ASSERT(userName != NULL);
    DebugPrint("DbUserDelete", "userName=\"%s\"\n", userName);

    if (!DbAcquire(&context)) {
        return FALSE;
    }

    DbRelease(context);
    return TRUE;
}

BOOL
DbUserLock(
    USERFILE *userFile
    )
{
    DB_CONTEXT *context;

    ASSERT(userFile != NULL);
    DebugPrint("DbUserLock", "userFile=%p", userFile);

    if (!DbAcquire(&context)) {
        return FALSE;
    }

    DbRelease(context);
    return TRUE;
}

BOOL
DbUserUnlock(
    USERFILE *userFile
    )
{
    DB_CONTEXT *context;

    ASSERT(userFile != NULL);
    DebugPrint("DbUserUnlock", "userFile=%p", userFile);

    if (!DbAcquire(&context)) {
        return FALSE;
    }

    DbRelease(context);
    return TRUE;
}

BOOL
DbUserOpen(
    char *userName,
    USERFILE *userFile
    )
{
    DB_CONTEXT *context;

    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    DebugPrint("DbUserOpen", "userName=\"%s\" userFile=%p\n", userName, userFile);

    if (!DbAcquire(&context)) {
        return FALSE;
    }

    DbRelease(context);
    return TRUE;
}

BOOL
DbUserWrite(
    USERFILE *userFile
    )
{
    DB_CONTEXT *context;

    ASSERT(userFile != NULL);
    DebugPrint("DbUserWrite", "userFile=%p", userFile);

    if (!DbAcquire(&context)) {
        return FALSE;
    }

    DbRelease(context);
    return TRUE;
}

BOOL
DbUserClose(
    USER_CONTEXT *context
    )
{
    ASSERT(context != NULL);
    DebugPrint("DbUserClose", "context=%p", context);

    return TRUE;
}

BOOL
DbUserRefresh(
    DB_CONTEXT *context
    )
{
    ASSERT(context != NULL);
    DebugPrint("DbUserRefresh", "context=%p", context);

    return TRUE;
}
