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
    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);
    DebugPrint("DbGroupCreate", "groupName=\"%s\" groupFile=%p\n", groupName, groupFile);

    return TRUE;
}

BOOL
DbGroupRename(
    char *groupName,
    char *newName
    )
{
    ASSERT(groupName != NULL);
    ASSERT(newName != NULL);
    DebugPrint("DbGroupRename", "groupName=\"%s\" newName=\"%s\"\n", groupName, newName);

    return TRUE;
}

BOOL
DbGroupDelete(
    char *groupName
    )
{
    ASSERT(groupName != NULL);
    DebugPrint("DbGroupDelete", "groupName=\"%s\"\n", groupName);

    return TRUE;
}

BOOL
DbGroupLock(
    GROUPFILE *groupFile
    )
{
    ASSERT(groupFile != NULL);
    DebugPrint("DbGroupLock", "groupFile=%p", groupFile);

    return TRUE;
}

BOOL
DbGroupUnlock(
    GROUPFILE *groupFile
    )
{
    ASSERT(groupFile != NULL);
    DebugPrint("DbGroupUnlock", "groupFile=%p", groupFile);

    return TRUE;
}

BOOL
DbGroupOpen(
    char *groupName,
    GROUPFILE *groupFile
    )
{
    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);
    DebugPrint("DbGroupOpen", "groupName=\"%s\" groupFile=%p\n", groupName, groupFile);

    return TRUE;
}

BOOL
DbGroupWrite(
    GROUPFILE *groupFile
    )
{
    ASSERT(groupFile != NULL);
    DebugPrint("DbGroupWrite", "groupFile=%p", groupFile);

    return TRUE;
}

BOOL
DbGroupClose(
    INT_CONTEXT *context
    )
{
    ASSERT(context != NULL);
    DebugPrint("DbGroupClose", "context=%p", context);

    return TRUE;
}
