/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    Group Database Backend

Author:
    neoxed (neoxed@gmail.com) Jun 5, 2006

Abstract:
    Group database storage backend.

*/

#include "mydb.h"

DWORD DbGroupCreate(DB_CONTEXT *dbContext, CHAR *groupName, GROUPFILE *groupFile)
{
    ASSERT(dbContext != NULL);
    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbGroupRename(DB_CONTEXT *dbContext, CHAR *groupName, CHAR *newName)
{
    ASSERT(dbContext != NULL);
    ASSERT(groupName != NULL);
    ASSERT(newName != NULL);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbGroupDelete(DB_CONTEXT *dbContext, CHAR *groupName)
{
    ASSERT(dbContext != NULL);
    ASSERT(groupName != NULL);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbGroupLock(DB_CONTEXT *dbContext, GROUPFILE *groupFile)
{
    ASSERT(dbContext != NULL);
    ASSERT(groupFile != NULL);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbGroupUnlock(DB_CONTEXT *dbContext, GROUPFILE *groupFile)
{
    ASSERT(dbContext != NULL);
    ASSERT(groupFile != NULL);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbGroupOpen(DB_CONTEXT *dbContext, CHAR *groupName, GROUPFILE *groupFile)
{
    ASSERT(dbContext != NULL);
    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbGroupWrite(DB_CONTEXT *dbContext, GROUPFILE *groupFile)
{
    ASSERT(dbContext != NULL);
    ASSERT(groupFile != NULL);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbGroupClose(GROUPFILE *groupFile)
{
    ASSERT(groupFile != NULL);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbGroupRefresh(DB_CONTEXT *dbContext)
{
    ASSERT(dbContext != NULL);

    return ERROR_INTERNAL_ERROR;
}
