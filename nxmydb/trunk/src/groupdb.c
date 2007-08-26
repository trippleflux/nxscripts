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
DbGroupRefresh(
    DB_CONTEXT *dbContext
    )
{
    ASSERT(dbContext != NULL);

    return TRUE;
}

BOOL
DbGroupCreate(
    DB_CONTEXT *dbContext,
    char *groupName,
    GROUPFILE *groupFile
    )
{
    ASSERT(dbContext != NULL);
    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);

    return TRUE;
}

BOOL
DbGroupRename(
    DB_CONTEXT *dbContext,
    char *groupName,
    char *newName
    )
{
    ASSERT(dbContext != NULL);
    ASSERT(groupName != NULL);
    ASSERT(newName != NULL);

    return TRUE;
}

BOOL
DbGroupDelete(
    DB_CONTEXT *dbContext,
    char *groupName
    )
{
    ASSERT(dbContext != NULL);
    ASSERT(groupName != NULL);

    return TRUE;
}

BOOL
DbGroupLock(
    DB_CONTEXT *dbContext,
    GROUPFILE *groupFile
    )
{
    ASSERT(dbContext != NULL);
    ASSERT(groupFile != NULL);

    return TRUE;
}

BOOL
DbGroupUnlock(
    DB_CONTEXT *dbContext,
    GROUPFILE *groupFile
    )
{
    ASSERT(dbContext != NULL);
    ASSERT(groupFile != NULL);

    return TRUE;
}

BOOL
DbGroupOpen(
    DB_CONTEXT *dbContext,
    char *groupName,
    GROUPFILE *groupFile
    )
{
    ASSERT(dbContext != NULL);
    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);

    return TRUE;
}

BOOL
DbGroupWrite(
    DB_CONTEXT *dbContext,
    GROUPFILE *groupFile
    )
{
    ASSERT(dbContext != NULL);
    ASSERT(groupFile != NULL);

    return TRUE;
}

BOOL
DbGroupClose(
    DB_CONTEXT *dbContext
    )
{
    ASSERT(dbContext != NULL);

    return TRUE;
}
