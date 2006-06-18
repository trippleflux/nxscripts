/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Group Database Backend

Author:
    neoxed (neoxed@gmail.com) Jun 5, 2006

Abstract:
    Group database storage backend.

*/

#include "mydb.h"

BOOL
DbGroupCreate(
    char *groupName,
    GROUPFILE *groupFile
    )
{
    DB_CONTEXT *context;

    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);
    DebugPrint("DbGroupCreate", "groupName=\"%s\" groupFile=%p\n", groupName, groupFile);

    if (!DbAcquire(&context)) {
        return FALSE;
    }

    DbRelease(context);
    return TRUE;
}

BOOL
DbGroupRename(
    char *groupName,
    char *newName
    )
{
    DB_CONTEXT *context;

    ASSERT(groupName != NULL);
    ASSERT(newName != NULL);
    DebugPrint("DbGroupRename", "groupName=\"%s\" newName=\"%s\"\n", groupName, newName);

    if (!DbAcquire(&context)) {
        return FALSE;
    }

    DbRelease(context);
    return TRUE;
}

BOOL
DbGroupDelete(
    char *groupName
    )
{
    DB_CONTEXT *context;

    ASSERT(groupName != NULL);
    DebugPrint("DbGroupDelete", "groupName=\"%s\"\n", groupName);

    if (!DbAcquire(&context)) {
        return FALSE;
    }

    DbRelease(context);
    return TRUE;
}

BOOL
DbGroupLock(
    GROUPFILE *groupFile
    )
{
    DB_CONTEXT *context;

    ASSERT(groupFile != NULL);
    DebugPrint("DbGroupLock", "groupFile=%p", groupFile);

    if (!DbAcquire(&context)) {
        return FALSE;
    }

    DbRelease(context);
    return TRUE;
}

BOOL
DbGroupUnlock(
    GROUPFILE *groupFile
    )
{
    DB_CONTEXT *context;

    ASSERT(groupFile != NULL);
    DebugPrint("DbGroupUnlock", "groupFile=%p", groupFile);

    if (!DbAcquire(&context)) {
        return FALSE;
    }

    DbRelease(context);
    return TRUE;
}

BOOL
DbGroupOpen(
    char *groupName,
    GROUPFILE *groupFile
    )
{
    DB_CONTEXT *context;

    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);
    DebugPrint("DbGroupOpen", "groupName=\"%s\" groupFile=%p\n", groupName, groupFile);

    if (!DbAcquire(&context)) {
        return FALSE;
    }

    DbRelease(context);
    return TRUE;
}

BOOL
DbGroupWrite(
    GROUPFILE *groupFile
    )
{
    DB_CONTEXT *context;

    ASSERT(groupFile != NULL);
    DebugPrint("DbGroupWrite", "groupFile=%p", groupFile);

    if (!DbAcquire(&context)) {
        return FALSE;
    }

    DbRelease(context);
    return TRUE;
}

BOOL
DbGroupClose(
    GROUP_CONTEXT *context
    )
{
    ASSERT(context != NULL);
    DebugPrint("DbGroupClose", "context=%p", context);

    return TRUE;
}

BOOL
DbGroupRefresh(
    DB_CONTEXT *context
    )
{
    ASSERT(context != NULL);
    DebugPrint("DbGroupRefresh", "context=%p", context);

    return TRUE;
}
