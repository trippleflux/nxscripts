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
    mod = MemAllocate(sizeof(MOD_CONTEXT));
    if (mod == NULL) {
        result = ERROR_NOT_ENOUGH_MEMORY;
        TRACE("Unable to allocate module context.");

    } else {
        // Initialize MOD_CONTEXT structure
        mod->file = INVALID_HANDLE_VALUE;
        groupFile->lpInternal = mod;

        // Register group
        result = GroupRegister(groupName, groupFile, &groupId);
        if (result != ERROR_SUCCESS) {
            TRACE("Unable to register group (error %lu).", result);
        } else {

            // Create group file
            result = FileGroupCreate(groupId, groupFile);
            if (result != ERROR_SUCCESS) {
                TRACE("Unable to create group file (error %lu).", result);

                // Creation failed, clean-up the group file
                FileGroupDelete(groupId);
                FileGroupClose(groupFile);
                GroupUnregister(groupName);
            }
        }

        if (result != ERROR_SUCCESS) {
            // Free module context after all file operations
            MemFree(mod);
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

static DWORD EventDeleteEx(CHAR *groupName, INT32 groupId)
{
    DWORD result;

    ASSERT(groupName != NULL);

    // Delete group file (success does not matter)
    result = FileGroupDelete(groupId);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to delete group file (error %lu).", result);
    }

    // Unregister group
    result = GroupUnregister(groupName);

    return result;
}

static DWORD EventDelete(CHAR *groupName)
{
    DWORD result;
    INT32 groupId;

    ASSERT(groupName != NULL);

    // Resolve group name to ID
    groupId = Io_Group2Gid(groupName);
    if (groupId == -1) {
        result = GetLastError();
        ASSERT(result != ERROR_SUCCESS);

        TRACE("Unable to resolve group \"%s\" (error %lu).", groupName, result);
    } else {
        // Delete group
        result = EventDeleteEx(groupName, groupId);
    }

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


static DWORD GroupSyncFull(DB_CONTEXT *db)
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
    TRACE("db=%p", db);

    //
    // Build list of group IDs
    //

    error = NameListCreateGroups(&list);
    if (error != ERROR_SUCCESS) {
        LOG_ERROR("Unable to create group ID list (error %lu).", error);
        return error;
    }

    //
    // Prepare and execute statement
    //

    stmt = db->stmt[7];

    query = "SELECT name, description, slots, users, vfsfile"
            "  FROM io_group";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    metadata = mysql_stmt_result_metadata(stmt);
    if (metadata == NULL) {
        TRACE("Unable to prepare statement: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    result = mysql_stmt_execute(stmt);
    if (result != 0) {
        TRACE("Unable to execute statement: %s", mysql_stmt_error(stmt));
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
        TRACE("Unable to bind results: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    result = mysql_stmt_store_result(stmt);
    if (result != 0) {
        TRACE("Unable to buffer results: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    //
    // Process result set
    //

    for (;;) {
        ZeroMemory(&groupFile, sizeof(GROUPFILE));
        if (mysql_stmt_fetch(stmt) != 0) {
            break;
        }

        if (!NameListRemove(&list, groupName)) {
            TRACE("GroupSyncFull: Create(%s)", groupName);

            // Group does not exist locally, create it.
            error = EventCreate(groupName, &groupFile);
            if (error != ERROR_SUCCESS) {
                LOG_WARN("Unable to create group \"%s\" (error %lu).", groupName, error);
            }
        } else {
            TRACE("GroupSyncFull: Update(%s)", groupName);

            // Group already exists locally, update it.
            error = EventUpdate(groupName, &groupFile);
            if (error != ERROR_SUCCESS) {
                LOG_WARN("Unable to update group \"%s\" (error %lu).", groupName, error);
            }
        }
    }

    mysql_free_result(metadata);

    //
    // Delete remaining groups
    //

    for (i = 0; i < list.count; i++) {
        entry = list.array[i];
        TRACE("GroupSyncFull: Delete(%s,%d)", entry->name, entry->id);

        // Group does not exist on database, delete it.
        error = EventDeleteEx(entry->name, entry->id);
        if (error != ERROR_SUCCESS) {
            LOG_WARN("Unable to delete group \"%s\" (error %lu).", entry->name, error);
        }
    }

    NameListDestroy(&list);

    return ERROR_SUCCESS;
}

static DWORD GroupSyncIncrChanges(DB_CONTEXT *db, SYNC_CONTEXT *sync)
{
    CHAR        *query;
    BYTE        syncEvent;
    CHAR        syncInfo[255];
    CHAR        groupName[_MAX_NAME + 1];
    DWORD       error;
    GROUPFILE   groupFile;
    INT         result;
    ULONG       outputLength;
    MYSQL_BIND  bindInput[2];
    MYSQL_BIND  bindOutput[3];
    MYSQL_RES   *metadata;
    MYSQL_STMT  *stmt;

    ASSERT(db != NULL);
    ASSERT(sync != NULL);

    //
    // Prepare statement and bind parameters
    //

    stmt = db->stmt[7];

    query = "SELECT name, type, info FROM io_group_changes"
            "  WHERE time BETWEEN ? AND ?"
            "  ORDER BY id ASC";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    DB_CHECK_PARAMS(bindInput, stmt);
    ZeroMemory(&bindInput, sizeof(bindInput));

    bindInput[0].buffer_type = MYSQL_TYPE_LONG;
    bindInput[0].buffer      = &sync->prevUpdate;
    bindInput[0].is_unsigned = TRUE;

    bindInput[1].buffer_type = MYSQL_TYPE_LONG;
    bindInput[1].buffer      = &sync->currUpdate;
    bindInput[1].is_unsigned = TRUE;

    result = mysql_stmt_bind_param(stmt, bindInput);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    metadata = mysql_stmt_result_metadata(stmt);
    if (metadata == NULL) {
        TRACE("Unable to prepare statement: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    //
    // Execute prepared statement
    //

    result = mysql_stmt_execute(stmt);
    if (result != 0) {
        TRACE("Unable to execute statement: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    //
    // Bind and fetch results
    //

    DB_CHECK_RESULTS(bindOutput, metadata);
    ZeroMemory(&bindOutput, sizeof(bindOutput));

    bindOutput[0].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[0].buffer        = groupName;
    bindOutput[0].buffer_length = sizeof(groupName);
    bindOutput[0].length        = &outputLength;

    bindOutput[1].buffer_type   = MYSQL_TYPE_TINY;
    bindOutput[1].buffer        = &syncEvent;
    bindOutput[1].is_unsigned   = TRUE;

    bindOutput[2].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[2].buffer        = syncInfo;
    bindOutput[2].buffer_length = sizeof(syncInfo);
    bindOutput[2].length        = &outputLength;

    result = mysql_stmt_bind_result(stmt, bindOutput);
    if (result != 0) {
        TRACE("Unable to bind results: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    result = mysql_stmt_store_result(stmt);
    if (result != 0) {
        TRACE("Unable to buffer results: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    //
    // Process result set
    //

    for (;;) {
        if (mysql_stmt_fetch(stmt) != 0) {
            break;
        }

        switch ((SYNC_EVENT)syncEvent) {
            case SYNC_EVENT_CREATE:
                TRACE("GroupSyncIncr: Create(%s)", groupName);

                // Read group file from database
                ZeroMemory(&groupFile, sizeof(GROUPFILE));
                error = DbGroupRead(db, groupName, &groupFile);
                if (error != ERROR_SUCCESS) {
                    LOG_WARN("Unable to read group \"%s\" (error %lu).", groupName, error);
                } else {

                    // Create local user
                    error = EventCreate(groupName, &groupFile);
                    if (error != ERROR_SUCCESS) {
                        LOG_WARN("Unable to create group \"%s\" (error %lu).", groupName, error);
                    }
                }
                break;

            case SYNC_EVENT_RENAME:
                TRACE("GroupSyncIncr: Rename(%s,%s)", groupName, syncInfo);

                error = EventRename(groupName, syncInfo);
                if (error != ERROR_SUCCESS) {
                    LOG_WARN("Unable to rename group \"%s\" to \"%s\" (error %lu).", groupName, syncInfo, error);
                }
                break;

            case SYNC_EVENT_DELETE:
                TRACE("GroupSyncIncr: Delete(%s)", groupName);

                error = EventDelete(groupName);
                if (error != ERROR_SUCCESS) {
                    LOG_WARN("Unable to delete group \"%s\" (error %lu).", groupName, error);
                }
                break;

            default:
                LOG_ERROR("Unknown sync event %d.", syncEvent);
                break;
        }
    }

    mysql_free_result(metadata);

    return ERROR_SUCCESS;
}

static DWORD GroupSyncIncrUpdates(DB_CONTEXT *db, SYNC_CONTEXT *sync)
{
    CHAR        *query;
    CHAR        groupName[_MAX_NAME + 1];
    DWORD       error;
    GROUPFILE   groupFile;
    INT         result;
    ULONG       outputLength;
    MYSQL_BIND  bindInput[2];
    MYSQL_BIND  bindOutput[5];
    MYSQL_RES   *metadata;
    MYSQL_STMT  *stmt;

    ASSERT(db != NULL);
    ASSERT(sync != NULL);

    //
    // Prepare statement and bind parameters
    //

    stmt = db->stmt[7];

    query = "SELECT name, description, slots, users, vfsfile"
            "  FROM io_group"
            "  WHERE updated BETWEEN ? AND ?";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    DB_CHECK_PARAMS(bindInput, stmt);
    ZeroMemory(&bindInput, sizeof(bindInput));

    bindInput[0].buffer_type = MYSQL_TYPE_LONG;
    bindInput[0].buffer      = &sync->prevUpdate;
    bindInput[0].is_unsigned = TRUE;

    bindInput[1].buffer_type = MYSQL_TYPE_LONG;
    bindInput[1].buffer      = &sync->currUpdate;
    bindInput[1].is_unsigned = TRUE;

    result = mysql_stmt_bind_param(stmt, bindInput);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    metadata = mysql_stmt_result_metadata(stmt);
    if (metadata == NULL) {
        TRACE("Unable to prepare statement: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    //
    // Execute prepared statement
    //

    result = mysql_stmt_execute(stmt);
    if (result != 0) {
        TRACE("Unable to execute statement: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    //
    // Bind and fetch results
    //

    DB_CHECK_RESULTS(bindOutput, metadata);
    ZeroMemory(&bindOutput, sizeof(bindOutput));

    bindOutput[0].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[0].buffer        = groupName;
    bindOutput[0].buffer_length = sizeof(groupName);
    bindOutput[0].length        = &outputLength;

    bindOutput[1].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[1].buffer        = groupFile.szDescription;
    bindOutput[1].buffer_length = sizeof(groupFile.szDescription);
    bindOutput[1].length        = &outputLength;

    bindOutput[2].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[2].buffer        = groupFile.Slots;
    bindOutput[2].buffer_length = sizeof(groupFile.Slots);
    bindOutput[2].length        = &outputLength;

    bindOutput[3].buffer_type   = MYSQL_TYPE_LONG;
    bindOutput[3].buffer        = &groupFile.Users;

    bindOutput[4].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[4].buffer        = groupFile.szVfsFile;
    bindOutput[4].buffer_length = sizeof(groupFile.szVfsFile);
    bindOutput[4].length        = &outputLength;

    result = mysql_stmt_bind_result(stmt, bindOutput);
    if (result != 0) {
        TRACE("Unable to bind results: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    result = mysql_stmt_store_result(stmt);
    if (result != 0) {
        TRACE("Unable to buffer results: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    //
    // Process result set
    //

    for (;;) {
        ZeroMemory(&groupFile, sizeof(GROUPFILE));
        if (mysql_stmt_fetch(stmt) != 0) {
            break;
        }
        TRACE("GroupSyncIncr: Update(%s)", groupName);

        error = EventUpdate(groupName, &groupFile);
        if (error != ERROR_SUCCESS) {
            LOG_WARN("Unable to update group \"%s\" (error %lu).", groupName, error);
        }
    }

    mysql_free_result(metadata);

    return ERROR_SUCCESS;
}

static DWORD GroupSyncIncr(DB_CONTEXT *db, SYNC_CONTEXT *sync)
{
    DWORD result;

    ASSERT(db != NULL);
    ASSERT(sync != NULL);
    TRACE("db=%p sync=%p", db, sync);

    // Process events from the "io_group_changes" table
    result = GroupSyncIncrChanges(db, sync);
    if (result != ERROR_SUCCESS) {
        LOG_ERROR("Unable to sync incremental changes (error %lu).", result);
    }

    // Process updates from the "io_group" table
    result = GroupSyncIncrUpdates(db, sync);
    if (result != ERROR_SUCCESS) {
        LOG_ERROR("Unable to sync incremental updates (error %lu).", result);
    }

    return result;
}


DWORD DbGroupSync(DB_CONTEXT *db, SYNC_CONTEXT *sync)
{
    DWORD result;

    ASSERT(db != NULL);
    ASSERT(sync != NULL);
    TRACE("db=%p sync=%p", db, sync);

    if (sync->prevUpdate == 0) {
        // If there was no previous update time, we
        // perform a full group syncronization.
        result = GroupSyncFull(db);
    } else {
        ASSERT(sync->currUpdate != 0);
        result = GroupSyncIncr(db, sync);
    }

    return result;
}
