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
    MYSQL *handle;

    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);
    DebugPrint("DbGroupCreate", "groupName=\"%s\" groupFile=%p\n", groupName, groupFile);

    if (!DbAcquire(&handle)) {
        return FALSE;
    }

    DbRelease(handle);
    return TRUE;
}

BOOL
DbGroupRename(
    char *groupName,
    char *newName
    )
{
    MYSQL *handle;

    ASSERT(groupName != NULL);
    ASSERT(newName != NULL);
    DebugPrint("DbGroupRename", "groupName=\"%s\" newName=\"%s\"\n", groupName, newName);

    if (!DbAcquire(&handle)) {
        return FALSE;
    }

    DbRelease(handle);
    return TRUE;
}

BOOL
DbGroupDelete(
    char *groupName
    )
{
    MYSQL *handle;

    ASSERT(groupName != NULL);
    DebugPrint("DbGroupDelete", "groupName=\"%s\"\n", groupName);

    if (!DbAcquire(&handle)) {
        return FALSE;
    }

    DbRelease(handle);
    return TRUE;
}

BOOL
DbGroupLock(
    GROUPFILE *groupFile
    )
{
    MYSQL *handle;

    ASSERT(groupFile != NULL);
    DebugPrint("DbGroupLock", "groupFile=%p", groupFile);

    if (!DbAcquire(&handle)) {
        return FALSE;
    }

    DbRelease(handle);
    return TRUE;
}

BOOL
DbGroupUnlock(
    GROUPFILE *groupFile
    )
{
    MYSQL *handle;

    ASSERT(groupFile != NULL);
    DebugPrint("DbGroupUnlock", "groupFile=%p", groupFile);

    if (!DbAcquire(&handle)) {
        return FALSE;
    }

    DbRelease(handle);
    return TRUE;
}

BOOL
DbGroupOpen(
    char *groupName,
    GROUPFILE *groupFile
    )
{
    MYSQL *handle;

    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);
    DebugPrint("DbGroupOpen", "groupName=\"%s\" groupFile=%p\n", groupName, groupFile);

    if (!DbAcquire(&handle)) {
        return FALSE;
    }

    DbRelease(handle);
    return TRUE;
}

BOOL
DbGroupWrite(
    GROUPFILE *groupFile
    )
{
    MYSQL *handle;

    ASSERT(groupFile != NULL);
    DebugPrint("DbGroupWrite", "groupFile=%p", groupFile);

    if (!DbAcquire(&handle)) {
        return FALSE;
    }

    DbRelease(handle);
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
    MYSQL *handle
    )
{
    ASSERT(handle != NULL);
    DebugPrint("DbGroupRefresh", "handle=%p", handle);

    return TRUE;
}
