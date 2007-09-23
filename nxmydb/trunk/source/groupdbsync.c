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

static DWORD EventCreate(CHAR *groupName, GROUPFILE *groupFile)
{
    MOD_CONTEXT *mod;
    DWORD       result;
    INT32       groupId;

    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);

    // Module context is required for all file operations
    mod = Io_Allocate(sizeof(MOD_CONTEXT));
    if (mod == NULL) {
        result = ERROR_NOT_ENOUGH_MEMORY;
        TRACE("Unable to allocate module context.\n");

    } else {
        // Initialize MOD_CONTEXT structure
        mod->file = INVALID_HANDLE_VALUE;
        groupFile->lpInternal = mod;

        // Register group
        result = GroupRegister(groupName, groupFile, &groupId);
        if (result != ERROR_SUCCESS) {
            TRACE("Unable to register group (error %lu).\n", result);
        } else {

            // Create group file
            result = FileGroupCreate(groupId, groupFile);
            if (result != ERROR_SUCCESS) {
                TRACE("Unable to create group file (error %lu).\n", result);

                //Creation failed, clean-up the group file
                FileGroupDelete(groupId);
                FileGroupClose(groupFile);
                GroupUnregister(groupName);
            }
        }

        if (result != ERROR_SUCCESS) {
            // Free module context after all file operations
            Io_Free(mod);
        }
    }

    return result;
}

static DWORD EventRename(CHAR *groupName, CHAR *newName)
{
    DWORD result;

    ASSERT(groupName != NULL);
    ASSERT(newName != NULL);

    // Register group under the new name
    result = GroupRegisterAs(groupName, newName);

    return result;
}

static DWORD EventDelete(CHAR *groupName, INT32 groupId)
{
    DWORD result;

    ASSERT(groupName != NULL);

    // Delete group file (success does not matter)
    result = FileGroupDelete(groupId);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to delete group file (error %lu).\n", result);
    }

    // Unregister group
    result = GroupUnregister(groupName);

    return result;
}

static DWORD EventUpdate(CHAR *groupName, GROUPFILE *groupFile)
{
    DWORD result;

    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);

    // Update the group file
    result = GroupUpdateByName(groupName, groupFile);

    return result;
}


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
            TRACE("GroupSyncFull: Create(%s)\n", groupName);

            // Group does not exist locally, create it.
            error = EventCreate(groupName, &groupFile);
            if (error != ERROR_SUCCESS) {
                TRACE("Unable to create group \"%s\" (error %lu).\n", groupName, error);
            }
        } else {
            TRACE("GroupSyncFull: Update(%s)\n", groupName);

            // Group already exists locally, update it.
            error = EventUpdate(groupName, &groupFile);
            if (error != ERROR_SUCCESS) {
                TRACE("Unable to update group \"%s\" (error %lu).\n", groupName, error);
            }
        }
    }

    mysql_free_result(metadata);

    //
    // Delete remaining groups
    //

    for (i = 0; i < list.count; i++) {
        entry = list.array[i];
        TRACE("GroupSyncFull: Delete(%s,%d)\n", entry->name, entry->id);

        // Group does not exist on database, delete it.
        error = EventDelete(entry->name, entry->id);
        if (error != ERROR_SUCCESS) {
            TRACE("Unable to delete group \"%s\" (error %lu).\n", entry->name, error);
        }
    }

    NameListDestroy(&list);

    return ERROR_SUCCESS;
}

static DWORD SyncIncremental(DB_CONTEXT *db, SYNC_CONTEXT *sync)
{
    ASSERT(db != NULL);
    ASSERT(sync != NULL);

    //
    // process io_group_changes table
    //  - creates/renames/deletes
    //  - done incrementally
    //
    // process io_group table
    //  - groupfile changes
    //  - done incrementally
    //

    return ERROR_SUCCESS;
}


DWORD DbGroupSync(DB_CONTEXT *db, SYNC_CONTEXT *sync)
{
    DWORD result;

    ASSERT(db != NULL);
    ASSERT(sync != NULL);
    TRACE("db=%p sync=%p\n", db, sync);

    if (sync->prevUpdate == 0) {
        // If there was no previous update time, we
        // perform a full group syncronization.
        result = SyncFull(db);
    } else {
        ASSERT(sync->currUpdate != 0);
        result = SyncIncremental(db, sync);
    }

    return result;
}
