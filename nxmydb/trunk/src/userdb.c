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
    MYSQL *handle;

    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    DebugPrint("DbUserCreate", "userName=\"%s\" userFile=%p\n", userName, userFile);

    if (!DbAcquire(&handle)) {
        return FALSE;
    }

    DbRelease(handle);
    return TRUE;
}

BOOL
DbUserRename(
    char *userName,
    char *newName
    )
{
    MYSQL *handle;

    ASSERT(userName != NULL);
    ASSERT(newName != NULL);
    DebugPrint("DbUserRename", "userName=\"%s\" newName=\"%s\"\n", userName, newName);

    if (!DbAcquire(&handle)) {
        return FALSE;
    }

    DbRelease(handle);
    return TRUE;
}

BOOL
DbUserDelete(
    char *userName
    )
{
    MYSQL *handle;

    ASSERT(userName != NULL);
    DebugPrint("DbUserDelete", "userName=\"%s\"\n", userName);

    if (!DbAcquire(&handle)) {
        return FALSE;
    }

    DbRelease(handle);
    return TRUE;
}

BOOL
DbUserLock(
    USERFILE *userFile
    )
{
    MYSQL *handle;

    ASSERT(userFile != NULL);
    DebugPrint("DbUserLock", "userFile=%p", userFile);

    if (!DbAcquire(&handle)) {
        return FALSE;
    }

    DbRelease(handle);
    return TRUE;
}

BOOL
DbUserUnlock(
    USERFILE *userFile
    )
{
    MYSQL *handle;

    ASSERT(userFile != NULL);
    DebugPrint("DbUserUnlock", "userFile=%p", userFile);

    if (!DbAcquire(&handle)) {
        return FALSE;
    }

    DbRelease(handle);
    return TRUE;
}

BOOL
DbUserOpen(
    char *userName,
    USERFILE *userFile
    )
{
    MYSQL *handle;

    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    DebugPrint("DbUserOpen", "userName=\"%s\" userFile=%p\n", userName, userFile);

    if (!DbAcquire(&handle)) {
        return FALSE;
    }

    DbRelease(handle);
    return TRUE;
}

BOOL
DbUserWrite(
    USERFILE *userFile
    )
{
    MYSQL *handle;

    ASSERT(userFile != NULL);
    DebugPrint("DbUserWrite", "userFile=%p", userFile);

    if (!DbAcquire(&handle)) {
        return FALSE;
    }

    DbRelease(handle);
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
    MYSQL *handle
    )
{
    ASSERT(handle != NULL);
    DebugPrint("DbUserRefresh", "handle=%p", handle);

    return TRUE;
}
