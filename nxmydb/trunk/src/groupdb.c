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

    return TRUE;
}

BOOL
DbGroupRefresh(
    DB_CONTEXT *context
    )
{
    ASSERT(context != NULL);

    return TRUE;
}
