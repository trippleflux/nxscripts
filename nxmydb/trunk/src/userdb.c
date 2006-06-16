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
    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    DebugPrint("DbUserCreate", "userName=\"%s\" userFile=%p\n", userName, userFile);

    return TRUE;
}

BOOL
DbUserRename(
    char *userName,
    char *newName
    )
{
    ASSERT(userName != NULL);
    ASSERT(newName != NULL);
    DebugPrint("DbUserRename", "userName=\"%s\" newName=\"%s\"\n", userName, newName);

    return TRUE;
}

BOOL
DbUserDelete(
    char *userName
    )
{
    ASSERT(userName != NULL);
    DebugPrint("DbUserDelete", "userName=\"%s\"\n", userName);

    return TRUE;
}

BOOL
DbUserLock(
    USERFILE *userFile
    )
{
    ASSERT(userFile != NULL);
    DebugPrint("DbUserLock", "userFile=%p", userFile);

    return TRUE;
}

BOOL
DbUserUnlock(
    USERFILE *userFile
    )
{
    ASSERT(userFile != NULL);
    DebugPrint("DbUserUnlock", "userFile=%p", userFile);

    return TRUE;
}

BOOL
DbUserOpen(
    char *userName,
    USERFILE *userFile
    )
{
    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    DebugPrint("DbUserOpen", "userName=\"%s\" userFile=%p\n", userName, userFile);

    return TRUE;
}

BOOL
DbUserWrite(
    USERFILE *userFile
    )
{
    ASSERT(userFile != NULL);
    DebugPrint("DbUserWrite", "userFile=%p", userFile);

    return TRUE;
}

BOOL
DbUserClose(
    FILE_CONTEXT *context
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
