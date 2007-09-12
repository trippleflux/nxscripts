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

static DWORD SyncFull(DB_CONTEXT *db)
{
    ASSERT(db != NULL);

    //
    // build hash table of local groups
    //  - use GetGroups for an array of IDs and resolve them
    //  - consider using a faster method (RB tree)
    //
    // process io_group table
    //  - create group if not in the hash table
    //  - update group if in the hash table
    //  - remove each group from the hash table as processed
    //
    // process hash table
    //  - delete groups remaining in the hash table
    //

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
