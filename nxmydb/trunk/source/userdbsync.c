/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    User Database Sync

Author:
    neoxed (neoxed@gmail.com) Sep 9, 2007

Abstract:
    User database synchronization.

*/

#include <base.h>
#include <backends.h>
#include <database.h>
#include <namelist.h>

static DWORD EventCreate(CHAR *userName, USERFILE *userFile)
{
    MOD_CONTEXT *mod;
    DWORD       result;
    INT32       userId;

    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);

    // Module context is required for all file operations
    mod = MemAllocate(sizeof(MOD_CONTEXT));
    if (mod == NULL) {
        result = ERROR_NOT_ENOUGH_MEMORY;
        TRACE("Unable to allocate module context.\n");

    } else {
        // Initialize MOD_CONTEXT structure
        mod->file = INVALID_HANDLE_VALUE;
        userFile->lpInternal = mod;

        // Register user
        result = UserRegister(userName, userFile, &userId);
        if (result != ERROR_SUCCESS) {
            TRACE("Unable to register user (error %lu).\n", result);
        } else {

            // Create user file
            result = FileUserCreate(userId, userFile);
            if (result != ERROR_SUCCESS) {
                TRACE("Unable to create user file (error %lu).\n", result);

                // Creation failed, clean-up the user file
                FileUserDelete(userId);
                FileUserClose(userFile);
                UserUnregister(userName);
            }
        }

        if (result != ERROR_SUCCESS) {
            // Free module context after all file operations
            MemFree(mod);
        }
    }

    return result;
}

static DWORD EventRename(CHAR *userName, CHAR *newName)
{
    DWORD result;

    ASSERT(userName != NULL);
    ASSERT(newName != NULL);

    // Register user under the new name
    result = UserRegisterAs(userName, newName);

    return result;
}

static DWORD EventDeleteEx(CHAR *userName, INT32 userId)
{
    DWORD result;

    ASSERT(userName != NULL);

    // Delete user file (success does not matter)
    result = FileUserDelete(userId);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to delete user file (error %lu).\n", result);
    }

    // Unregister user
    result = UserUnregister(userName);

    return result;
}

static DWORD EventDelete(CHAR *userName)
{
    DWORD result;
    INT32 userId;

    ASSERT(userName != NULL);

    // Resolve user name to ID
    userId = Io_User2Uid(userName);
    if (userId == -1) {
        result = GetLastError();
        ASSERT(result != ERROR_SUCCESS);

        TRACE("Unable to resolve user \"%s\" (error %lu).\n", userName, result);
    } else {
        // Delete user
        result = EventDeleteEx(userName, userId);
    }

    return result;
}

static DWORD EventUpdate(CHAR *userName, USERFILE *userFile)
{
    DWORD result;

    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);

    // Update the user file
    result = UserUpdateByName(userName, userFile);

    return result;
}


static DWORD SyncFull(DB_CONTEXT *db)
{
    BOOL        removed;
    CHAR        *query;
    CHAR        userName[_MAX_NAME + 1];
    DWORD       error;
    DWORD       i;
    USERFILE    userFile;
    INT         result;
    NAME_ENTRY  *entry;
    NAME_LIST   list;
    ULONG       outputLength;
    MYSQL_BIND  bind[17];
    MYSQL_RES   *metadata;
    MYSQL_STMT  *stmt;

    ASSERT(db != NULL);
    TRACE("db=%p\n", db);

    //
    // Build list of user IDs
    //

    error = NameListCreateUsers(&list);
    if (error != ERROR_SUCCESS) {
        TRACE("Unable to create user ID list (error %lu).\n", error);
        return error;
    }

    //
    // Prepare and execute statement
    //

    stmt = db->stmt[7];

    query = "SELECT name,description,flags,home,limits,password,vfsfile,credits,"
            "       ratio,alldn,allup,daydn,dayup,monthdn,monthup,wkdn,wkup"
            "  FROM io_user";

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
    bind[0].buffer        = userName;
    bind[0].buffer_length = sizeof(userName);
    bind[0].length        = &outputLength;

    bind[1].buffer_type   = MYSQL_TYPE_STRING;
    bind[1].buffer        = userFile.Tagline;
    bind[1].buffer_length = sizeof(userFile.Tagline);
    bind[1].length        = &outputLength;

    bind[2].buffer_type   = MYSQL_TYPE_STRING;
    bind[2].buffer        = userFile.Flags;
    bind[2].buffer_length = sizeof(userFile.Flags);
    bind[2].length        = &outputLength;

    bind[3].buffer_type   = MYSQL_TYPE_STRING;
    bind[3].buffer        = userFile.Home;
    bind[3].buffer_length = sizeof(userFile.Home);
    bind[3].length        = &outputLength;

    bind[4].buffer_type   = MYSQL_TYPE_BLOB;
    bind[4].buffer        = &userFile.Limits;
    bind[4].buffer_length = sizeof(userFile.Limits);
    bind[4].length        = &outputLength;

    bind[5].buffer_type   = MYSQL_TYPE_BLOB;
    bind[5].buffer        = &userFile.Password;
    bind[5].buffer_length = sizeof(userFile.Password);
    bind[5].length        = &outputLength;

    bind[6].buffer_type   = MYSQL_TYPE_STRING;
    bind[6].buffer        = userFile.MountFile;
    bind[6].buffer_length = sizeof(userFile.MountFile);
    bind[6].length        = &outputLength;

    bind[7].buffer_type   = MYSQL_TYPE_BLOB;
    bind[7].buffer        = &userFile.Ratio;
    bind[7].buffer_length = sizeof(userFile.Ratio);
    bind[7].length        = &outputLength;

    bind[8].buffer_type   = MYSQL_TYPE_BLOB;
    bind[8].buffer        = &userFile.Credits;
    bind[8].buffer_length = sizeof(userFile.Credits);
    bind[8].length        = &outputLength;

    bind[9].buffer_type   = MYSQL_TYPE_BLOB;
    bind[9].buffer        = &userFile.DayUp;
    bind[9].buffer_length = sizeof(userFile.DayUp);
    bind[9].length        = &outputLength;

    bind[10].buffer_type   = MYSQL_TYPE_BLOB;
    bind[10].buffer        = &userFile.DayDn;
    bind[10].buffer_length = sizeof(userFile.DayDn);
    bind[10].length        = &outputLength;

    bind[11].buffer_type   = MYSQL_TYPE_BLOB;
    bind[11].buffer        = &userFile.WkUp;
    bind[11].buffer_length = sizeof(userFile.WkUp);
    bind[11].length        = &outputLength;

    bind[12].buffer_type   = MYSQL_TYPE_BLOB;
    bind[12].buffer        = &userFile.WkDn;
    bind[12].buffer_length = sizeof(userFile.WkDn);
    bind[12].length        = &outputLength;

    bind[13].buffer_type   = MYSQL_TYPE_BLOB;
    bind[13].buffer        = &userFile.MonthUp;
    bind[13].buffer_length = sizeof(userFile.MonthUp);
    bind[13].length        = &outputLength;

    bind[14].buffer_type   = MYSQL_TYPE_BLOB;
    bind[14].buffer        = &userFile.MonthDn;
    bind[14].buffer_length = sizeof(userFile.MonthDn);
    bind[14].length        = &outputLength;

    bind[15].buffer_type   = MYSQL_TYPE_BLOB;
    bind[15].buffer        = &userFile.AllUp;
    bind[15].buffer_length = sizeof(userFile.AllUp);
    bind[15].length        = &outputLength;

    bind[16].buffer_type   = MYSQL_TYPE_BLOB;
    bind[16].buffer        = &userFile.AllDn;
    bind[16].buffer_length = sizeof(userFile.AllDn);
    bind[16].length        = &outputLength;

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

    //
    // Process result set
    //

    for (;;) {
        ZeroMemory(&userFile, sizeof(USERFILE));
        if (mysql_stmt_fetch(stmt) != 0) {
            break;
        }

        // Remove user from the list beforehand in case DbUserReadExtra() fails
        removed = NameListRemove(&list, userName);

        // Read the user's admin-groups, groups, and hosts
        error = DbUserReadExtra(db, userName, &userFile);
        if (error != ERROR_SUCCESS) {
            TRACE("Unable to read user \"%s\" (error %lu).\n", userName, error);
            continue;
        }

        if (!removed) {
            TRACE("UserSyncFull: Create(%s)\n", userName);

            // User does not exist locally, create it.
            error = EventCreate(userName, &userFile);
            if (error != ERROR_SUCCESS) {
                TRACE("Unable to create user \"%s\" (error %lu).\n", userName, error);
            }
        } else {
            TRACE("UserSyncFull: Update(%s)\n", userName);

            // User already exists locally, update it.
            error = EventUpdate(userName, &userFile);
            if (error != ERROR_SUCCESS) {
                TRACE("Unable to update user \"%s\" (error %lu).\n", userName, error);
            }
        }
    }

    mysql_free_result(metadata);

    //
    // Delete remaining users
    //

    for (i = 0; i < list.count; i++) {
        entry = list.array[i];
        TRACE("UserSyncFull: Delete(%s,%d)\n", entry->name, entry->id);

        // User does not exist on database, delete it.
        error = EventDeleteEx(entry->name, entry->id);
        if (error != ERROR_SUCCESS) {
            TRACE("Unable to delete user \"%s\" (error %lu).\n", entry->name, error);
        }
    }

    NameListDestroy(&list);

    return ERROR_SUCCESS;
}

static DWORD SyncIncrChanges(DB_CONTEXT *db, SYNC_CONTEXT *sync)
{
    CHAR        *query;
    BYTE        syncEvent;
    CHAR        syncInfo[255];
    CHAR        userName[_MAX_NAME + 1];
    DWORD       error;
    USERFILE    userFile;
    INT         result;
    ULONG       outputLength;
    MYSQL_BIND  bindInput[2];
    MYSQL_BIND  bindOutput[3];
    MYSQL_RES   *metadata;
    MYSQL_STMT  *stmt;

    ASSERT(db != NULL);
    ASSERT(sync != NULL);
    TRACE("db=%p sync=%p\n", db, sync);

    //
    // Prepare statement and bind parameters
    //

    stmt = db->stmt[7];

    query = "SELECT name, type, info FROM io_user_changes"
            "  WHERE time BETWEEN ? AND ?"
            "  ORDER BY id ASC";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmt));
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
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    metadata = mysql_stmt_result_metadata(stmt);
    if (metadata == NULL) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    //
    // Execute prepared statement
    //

    result = mysql_stmt_execute(stmt);
    if (result != 0) {
        TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    //
    // Bind and fetch results
    //

    DB_CHECK_RESULTS(bindOutput, metadata);
    ZeroMemory(&bindOutput, sizeof(bindOutput));

    bindOutput[0].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[0].buffer        = userName;
    bindOutput[0].buffer_length = sizeof(userName);
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
        TRACE("Unable to bind results: %s\n", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    result = mysql_stmt_store_result(stmt);
    if (result != 0) {
        TRACE("Unable to buffer results: %s\n", mysql_stmt_error(stmt));
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
                TRACE("UserSyncIncr: Create(%s)\n", userName);

                // Read user file from database
                ZeroMemory(&userFile, sizeof(USERFILE));
                error = DbUserRead(db, userName, &userFile);
                if (error != ERROR_SUCCESS) {
                    TRACE("Unable to read user \"%s\" (error %lu).\n", userName, error);
                } else {

                    // Create local user
                    error = EventCreate(userName, &userFile);
                    if (error != ERROR_SUCCESS) {
                        TRACE("Unable to create user \"%s\" (error %lu).\n", userName, error);
                    }
                }
                break;

            case SYNC_EVENT_RENAME:
                TRACE("UserSyncIncr: Rename(%s,%s)\n", userName, syncInfo);

                error = EventRename(userName, syncInfo);
                if (error != ERROR_SUCCESS) {
                    TRACE("Unable to rename user \"%s\" to \"%s\" (error %lu).\n", userName, syncInfo, error);
                }
                break;

            case SYNC_EVENT_DELETE:
                TRACE("UserSyncIncr: Delete(%s)\n", userName);

                error = EventDelete(userName);
                if (error != ERROR_SUCCESS) {
                    TRACE("Unable to delete user \"%s\" (error %lu).\n", userName, error);
                }
                break;

            default:
                TRACE("Unknown sync event %d.\n", syncEvent);
                break;
        }
    }

    mysql_free_result(metadata);

    return ERROR_SUCCESS;
}

static DWORD SyncIncrUpdates(DB_CONTEXT *db, SYNC_CONTEXT *sync)
{
    CHAR        *query;
    CHAR        userName[_MAX_NAME + 1];
    DWORD       error;
    INT         result;
    ULONG       outputLength;
    USERFILE    userFile;
    MYSQL_BIND  bindInput[2];
    MYSQL_BIND  bindOutput[17];
    MYSQL_RES   *metadata;
    MYSQL_STMT  *stmt;

    ASSERT(db != NULL);
    ASSERT(sync != NULL);
    TRACE("db=%p sync=%p\n", db, sync);

    //
    // Prepare statement and bind parameters
    //

    stmt = db->stmt[7];

    query = "SELECT name,description,flags,home,limits,password,vfsfile,credits,"
            "       ratio,alldn,allup,daydn,dayup,monthdn,monthup,wkdn,wkup"
            "  FROM io_user"
            "  WHERE updated BETWEEN ? AND ?";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmt));
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
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    metadata = mysql_stmt_result_metadata(stmt);
    if (metadata == NULL) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    //
    // Execute prepared statement
    //

    result = mysql_stmt_execute(stmt);
    if (result != 0) {
        TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    //
    // Bind and fetch results
    //

    DB_CHECK_RESULTS(bindOutput, metadata);
    ZeroMemory(&bindOutput, sizeof(bindOutput));

    bindOutput[0].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[0].buffer        = userName;
    bindOutput[0].buffer_length = sizeof(userName);
    bindOutput[0].length        = &outputLength;

    bindOutput[1].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[1].buffer        = userFile.Tagline;
    bindOutput[1].buffer_length = sizeof(userFile.Tagline);
    bindOutput[1].length        = &outputLength;

    bindOutput[2].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[2].buffer        = userFile.Flags;
    bindOutput[2].buffer_length = sizeof(userFile.Flags);
    bindOutput[2].length        = &outputLength;

    bindOutput[3].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[3].buffer        = userFile.Home;
    bindOutput[3].buffer_length = sizeof(userFile.Home);
    bindOutput[3].length        = &outputLength;

    bindOutput[4].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[4].buffer        = &userFile.Limits;
    bindOutput[4].buffer_length = sizeof(userFile.Limits);
    bindOutput[4].length        = &outputLength;

    bindOutput[5].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[5].buffer        = &userFile.Password;
    bindOutput[5].buffer_length = sizeof(userFile.Password);
    bindOutput[5].length        = &outputLength;

    bindOutput[6].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[6].buffer        = userFile.MountFile;
    bindOutput[6].buffer_length = sizeof(userFile.MountFile);
    bindOutput[6].length        = &outputLength;

    bindOutput[7].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[7].buffer        = &userFile.Ratio;
    bindOutput[7].buffer_length = sizeof(userFile.Ratio);
    bindOutput[7].length        = &outputLength;

    bindOutput[8].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[8].buffer        = &userFile.Credits;
    bindOutput[8].buffer_length = sizeof(userFile.Credits);
    bindOutput[8].length        = &outputLength;

    bindOutput[9].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[9].buffer        = &userFile.DayUp;
    bindOutput[9].buffer_length = sizeof(userFile.DayUp);
    bindOutput[9].length        = &outputLength;

    bindOutput[10].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[10].buffer        = &userFile.DayDn;
    bindOutput[10].buffer_length = sizeof(userFile.DayDn);
    bindOutput[10].length        = &outputLength;

    bindOutput[11].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[11].buffer        = &userFile.WkUp;
    bindOutput[11].buffer_length = sizeof(userFile.WkUp);
    bindOutput[11].length        = &outputLength;

    bindOutput[12].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[12].buffer        = &userFile.WkDn;
    bindOutput[12].buffer_length = sizeof(userFile.WkDn);
    bindOutput[12].length        = &outputLength;

    bindOutput[13].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[13].buffer        = &userFile.MonthUp;
    bindOutput[13].buffer_length = sizeof(userFile.MonthUp);
    bindOutput[13].length        = &outputLength;

    bindOutput[14].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[14].buffer        = &userFile.MonthDn;
    bindOutput[14].buffer_length = sizeof(userFile.MonthDn);
    bindOutput[14].length        = &outputLength;

    bindOutput[15].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[15].buffer        = &userFile.AllUp;
    bindOutput[15].buffer_length = sizeof(userFile.AllUp);
    bindOutput[15].length        = &outputLength;

    bindOutput[16].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[16].buffer        = &userFile.AllDn;
    bindOutput[16].buffer_length = sizeof(userFile.AllDn);
    bindOutput[16].length        = &outputLength;

    result = mysql_stmt_bind_result(stmt, bindOutput);
    if (result != 0) {
        TRACE("Unable to bind results: %s\n", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    result = mysql_stmt_store_result(stmt);
    if (result != 0) {
        TRACE("Unable to buffer results: %s\n", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    //
    // Process result set
    //

    for (;;) {
        ZeroMemory(&userFile, sizeof(USERFILE));
        if (mysql_stmt_fetch(stmt) != 0) {
            break;
        }
        TRACE("UserSyncIncr: Update(%s)\n", userName);

        // Read the user's admin-groups, groups, and hosts
        error = DbUserReadExtra(db, userName, &userFile);
        if (error != ERROR_SUCCESS) {
            TRACE("Unable to read user \"%s\" (error %lu).\n", userName, error);
        } else {

            // Update user file
            error = EventUpdate(userName, &userFile);
            if (error != ERROR_SUCCESS) {
                TRACE("Unable to update user \"%s\" (error %lu).\n", userName, error);
            }
        }
    }

    mysql_free_result(metadata);

    return ERROR_SUCCESS;
}

static DWORD SyncIncr(DB_CONTEXT *db, SYNC_CONTEXT *sync)
{
    DWORD result;

    ASSERT(db != NULL);
    ASSERT(sync != NULL);
    TRACE("db=%p sync=%p\n", db, sync);

    // Process events from the "io_user_changes" table
    result = SyncIncrChanges(db, sync);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to sync incremental changes (error %lu).\n", result);
    }

    // Process updates from the "io_user" table
    result = SyncIncrUpdates(db, sync);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to sync incremental updates (error %lu).\n", result);
    }

    return result;
}


DWORD DbUserSync(DB_CONTEXT *db, SYNC_CONTEXT *sync)
{
    DWORD result;

    ASSERT(db != NULL);
    ASSERT(sync != NULL);
    TRACE("db=%p sync=%p\n", db, sync);

    if (sync->prevUpdate == 0) {
        // If there was no previous update time, we
        // perform a full user syncronization.
        result = SyncFull(db);
    } else {
        ASSERT(sync->currUpdate != 0);
        result = SyncIncr(db, sync);
    }

    return result;
}
