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
    INT32 groupId,
    GROUPFILE *groupFile
    )
{
    return TRUE;
}

BOOL
DbGroupRename(
    char *groupName,
    INT32 groupId,
    char *newName
    )
{
    return TRUE;
}

BOOL
DbGroupDelete(
    char *groupName,
    INT32 groupId
    )
{
    return TRUE;
}

BOOL
DbGroupLock(
    GROUPFILE *groupFile
    )
{
    return TRUE;
}

BOOL
DbGroupUnlock(
    GROUPFILE *groupFile
    )
{
    return TRUE;
}

BOOL
DbGroupOpen(
    char *groupName,
    GROUPFILE *groupFile
    )
{
    return TRUE;
}

BOOL
DbGroupWrite(
    GROUPFILE *groupFile
    )
{
    return TRUE;
}

BOOL
DbGroupClose(
    INT_CONTEXT *context
    )
{
    return TRUE;
}
