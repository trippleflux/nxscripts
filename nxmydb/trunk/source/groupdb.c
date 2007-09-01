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

#include <base.h>
#include <backends.h>
#include <database.h>

DWORD DbGroupCreate(DB_CONTEXT *db, CHAR *groupName, GROUPFILE *groupFile)
{
    ASSERT(db != NULL);
    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);
    TRACE("db=%p groupName=%s groupFile=%p\n", db, groupName, groupFile);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbGroupRename(DB_CONTEXT *db, CHAR *groupName, CHAR *newName)
{
    ASSERT(db != NULL);
    ASSERT(groupName != NULL);
    ASSERT(newName != NULL);
    TRACE("db=%p groupName=%s newName=%s\n", db, groupName, newName);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbGroupDelete(DB_CONTEXT *db, CHAR *groupName)
{
    ASSERT(db != NULL);
    ASSERT(groupName != NULL);
    TRACE("db=%p groupName=%s\n", db, groupName);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbGroupLock(DB_CONTEXT *db, CHAR *groupName, GROUPFILE *groupFile)
{
    ASSERT(db != NULL);
    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);
    TRACE("db=%p groupName=%s groupFile%=p\n", db, groupName, groupFile);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbGroupUnlock(DB_CONTEXT *db, CHAR *groupName)
{
    ASSERT(db != NULL);
    ASSERT(groupName != NULL);
    TRACE("db=%p groupName=%s\n", db, groupName);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbGroupOpen(DB_CONTEXT *db, CHAR *groupName, GROUPFILE *groupFile)
{
    ASSERT(db != NULL);
    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);
    TRACE("db=%p groupName=%s groupFile=%p\n", db, groupName, groupFile);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbGroupWrite(DB_CONTEXT *db, CHAR *groupName, GROUPFILE *groupFile)
{
    ASSERT(db != NULL);
    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);
    TRACE("db=%p groupName=%s groupFile%=p\n", db, groupName, groupFile);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbGroupClose(GROUPFILE *groupFile)
{
    ASSERT(groupFile != NULL);
    TRACE("groupFile=%p\n", groupFile);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbGroupRefresh(DB_CONTEXT *db)
{
    ASSERT(db != NULL);
    TRACE("db=%p\n", db);

    return ERROR_INTERNAL_ERROR;
}
