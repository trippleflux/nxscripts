/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    Group Database Sync

Author:
    neoxed (neoxed@gmail.com) Sep 9, 2007

Abstract:
    Group database synchronization.

*/

#include <base.h>
#include <backends.h>
#include <database.h>
#include <idlist.h>

static DWORD SyncFull(DB_CONTEXT *db)
{
    DWORD   result;
    ID_LIST list;

    ASSERT(db != NULL);

    result = IdListCreate(&list, ID_LIST_TYPE_GROUP);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to create group ID list (error %lu).\n", result);
        return result;
    }

    //
    // TODO
    //
    // process io_group table
    //  - select all data from the groups table
    //  - resolve group name to ID
    //  - look up group ID in the hash table and delete it
    //    - if group exists, update group
    //    - if group does not exist, register group
    //
    // process hash table
    //  - delete groups remaining in the hash table
    //

    IdListDestroy(&list);

    return ERROR_NOT_SUPPORTED;
}

static DWORD SyncIncremental(DB_CONTEXT *db, ULONG lastUpdate)
{
    ASSERT(db != NULL);
    ASSERT(lastUpdate > 0);

    //
    // process io_group_changes table
    //  - creates/renames/deletes
    //  - done incrementally
    //
    // process io_group table
    //  - groupfile changes
    //  - done incrementally
    //

    return ERROR_NOT_SUPPORTED;
}

DWORD DbGroupRefresh(DB_CONTEXT *db, ULONG lastUpdate)
{
    DWORD result;

    ASSERT(db != NULL);
    TRACE("db=%p lastUpdate=%lu\n", db, lastUpdate);

    if (lastUpdate) {
        result = SyncIncremental(db, lastUpdate);
    } else {
        result = SyncFull(db);
    }

    return result;
}
