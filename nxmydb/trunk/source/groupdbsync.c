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
#include <namelist.h>

static DWORD SyncFull(DB_CONTEXT *db)
{
    CHAR        *query;
    CHAR        groupName[_MAX_NAME + 1];
    DWORD       error;
    DWORD       i;
    GROUPFILE   groupFile;
    INT         result;
    NAME_ENTRY  *entry;
    NAME_LIST   list;
    ULONG       outputLength;
    MYSQL_BIND  bind[5];
    MYSQL_RES   *metadata;
    MYSQL_STMT  *stmt;

    ASSERT(db != NULL);

    //
    // Build list of group IDs
    //

    error = NameListCreateGroups(&list);
    if (error != ERROR_SUCCESS) {
        TRACE("Unable to create group ID list (error %lu).\n", error);
        return error;
    }

    //
    // Prepare and execute statement
    //

    stmt = db->stmt[7];

    query = "SELECT name, description, slots, users, vfsfile FROM io_group";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    metadata = mysql_stmt_result_metadata(stmt);
    if (metadata == NULL) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    result = mysql_stmt_execute(stmt);
    if (result != 0) {
        TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    //
    // Bind and fetch results
    //

    DB_CHECK_RESULTS(bind, metadata);
    ZeroMemory(&bind, sizeof(bind));

    bind[0].buffer_type   = MYSQL_TYPE_STRING;
    bind[0].buffer        = groupName;
    bind[0].buffer_length = sizeof(groupName);
    bind[0].length        = &outputLength;

    bind[1].buffer_type   = MYSQL_TYPE_STRING;
    bind[1].buffer        = groupFile.szDescription;
    bind[1].buffer_length = sizeof(groupFile.szDescription);
    bind[1].length        = &outputLength;

    bind[2].buffer_type   = MYSQL_TYPE_BLOB;
    bind[2].buffer        = groupFile.Slots;
    bind[2].buffer_length = sizeof(groupFile.Slots);
    bind[2].length        = &outputLength;

    bind[3].buffer_type   = MYSQL_TYPE_LONG;
    bind[3].buffer        = &groupFile.Users;

    bind[4].buffer_type   = MYSQL_TYPE_STRING;
    bind[4].buffer        = groupFile.szVfsFile;
    bind[4].buffer_length = sizeof(groupFile.szVfsFile);
    bind[4].length        = &outputLength;

    result = mysql_stmt_bind_result(stmt, bind);
    if (result != 0) {
        TRACE("Unable to bind results: %s\n", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    result = mysql_stmt_store_result(stmt);
    if (result != 0) {
        TRACE("Unable to buffer results: %s\n", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    for (;;) {
        ZeroMemory(&groupFile, sizeof(GROUPFILE));
        if (mysql_stmt_fetch(stmt) != 0) {
            break;
        }

        if (!NameListRemove(&list, groupName)) {
            // Group does not exist, create it.

            // TODO
            TRACE("GrpCrt %s\n", groupName);
        } else {
            // Group already exists, update it.

            // TODO
            TRACE("GrpUpd %s\n", groupName);
        }
    }

    mysql_free_result(metadata);

    //
    // Delete remaining groups
    //

    for (i = 0; i < list.count; i++) {
        entry = list.array[i];

        // TODO
        TRACE("GrpDel %s (%d)\n", entry->name, entry->id);
    }

    NameListDestroy(&list);

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
