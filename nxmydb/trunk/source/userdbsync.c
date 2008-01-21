/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2008 neoxed

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

static DWORD UserEventCreate(CHAR *userName, USERFILE *userFile)
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
        TRACE("Unable to allocate module context.");

    } else {
        // Initialize MOD_CONTEXT structure
        mod->file = INVALID_HANDLE_VALUE;
        userFile->lpInternal = mod;

        // Register user
        result = UserRegister(userName, userFile, &userId);
        if (result != ERROR_SUCCESS) {
            TRACE("Unable to register user (error %lu).", result);
        } else {

            // Create user file
            result = FileUserCreate(userId, userFile);
            if (result != ERROR_SUCCESS) {
                TRACE("Unable to create user file (error %lu).", result);

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

static DWORD UserEventRename(CHAR *userName, CHAR *newName)
{
    DWORD result;

    ASSERT(userName != NULL);
    ASSERT(newName != NULL);

    // Register user under the new name
    result = UserRegisterAs(userName, newName);

    return result;
}

static DWORD UserEventDeleteEx(CHAR *userName, INT32 userId)
{
    DWORD result;

    ASSERT(userName != NULL);

    // Delete user file (success does not matter)
    result = FileUserDelete(userId);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to delete user file (error %lu).", result);
    }

    // Unregister user
    result = UserUnregister(userName);

    return result;
}

static DWORD UserEventDelete(CHAR *userName)
{
    DWORD result;
    INT32 userId;

    ASSERT(userName != NULL);

    // Resolve user name to ID
    userId = Io_User2Uid(userName);
    if (userId == -1) {
        result = GetLastError();
        ASSERT(result != ERROR_SUCCESS);

        TRACE("Unable to resolve user \"%s\" (error %lu).", userName, result);
    } else {
        // Delete user
        result = UserEventDeleteEx(userName, userId);
    }

    return result;
}

static DWORD UserEventUpdate(CHAR *userName, USERFILE *userFile)
{
    DWORD result;

    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);

    // Update the user file
    result = UserUpdateByName(userName, userFile);

    return result;
}


static DWORD UserSyncFull(DB_CONTEXT *db)
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
    MYSQL_BIND  bind[17];
    MYSQL_RES   *metadata;
    MYSQL_STMT  *stmt;

    ASSERT(db != NULL);
    TRACE("db=%p", db);

    //
    // Build list of user IDs
    //

    error = NameListCreateUsers(&list);
    if (error != ERROR_SUCCESS) {
        LOG_ERROR("Unable to create user ID list (error %lu).", error);
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
        LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmt));
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

    bind[1].buffer_type   = MYSQL_TYPE_STRING;
    bind[1].buffer        = userFile.Tagline;
    bind[1].buffer_length = sizeof(userFile.Tagline);

    bind[2].buffer_type   = MYSQL_TYPE_STRING;
    bind[2].buffer        = userFile.Flags;
    bind[2].buffer_length = sizeof(userFile.Flags);

    bind[3].buffer_type   = MYSQL_TYPE_STRING;
    bind[3].buffer        = userFile.Home;
    bind[3].buffer_length = sizeof(userFile.Home);

    bind[4].buffer_type   = MYSQL_TYPE_BLOB;
    bind[4].buffer        = &userFile.Limits;
    bind[4].buffer_length = sizeof(userFile.Limits);

    bind[5].buffer_type   = MYSQL_TYPE_BLOB;
    bind[5].buffer        = &userFile.Password;
    bind[5].buffer_length = sizeof(userFile.Password);

    bind[6].buffer_type   = MYSQL_TYPE_STRING;
    bind[6].buffer        = userFile.MountFile;
    bind[6].buffer_length = sizeof(userFile.MountFile);

    bind[7].buffer_type   = MYSQL_TYPE_BLOB;
    bind[7].buffer        = &userFile.Ratio;
    bind[7].buffer_length = sizeof(userFile.Ratio);

    bind[8].buffer_type   = MYSQL_TYPE_BLOB;
    bind[8].buffer        = &userFile.Credits;
    bind[8].buffer_length = sizeof(userFile.Credits);

    bind[9].buffer_type   = MYSQL_TYPE_BLOB;
    bind[9].buffer        = &userFile.DayUp;
    bind[9].buffer_length = sizeof(userFile.DayUp);

    bind[10].buffer_type   = MYSQL_TYPE_BLOB;
    bind[10].buffer        = &userFile.DayDn;
    bind[10].buffer_length = sizeof(userFile.DayDn);

    bind[11].buffer_type   = MYSQL_TYPE_BLOB;
    bind[11].buffer        = &userFile.WkUp;
    bind[11].buffer_length = sizeof(userFile.WkUp);

    bind[12].buffer_type   = MYSQL_TYPE_BLOB;
    bind[12].buffer        = &userFile.WkDn;
    bind[12].buffer_length = sizeof(userFile.WkDn);

    bind[13].buffer_type   = MYSQL_TYPE_BLOB;
    bind[13].buffer        = &userFile.MonthUp;
    bind[13].buffer_length = sizeof(userFile.MonthUp);

    bind[14].buffer_type   = MYSQL_TYPE_BLOB;
    bind[14].buffer        = &userFile.MonthDn;
    bind[14].buffer_length = sizeof(userFile.MonthDn);

    bind[15].buffer_type   = MYSQL_TYPE_BLOB;
    bind[15].buffer        = &userFile.AllUp;
    bind[15].buffer_length = sizeof(userFile.AllUp);

    bind[16].buffer_type   = MYSQL_TYPE_BLOB;
    bind[16].buffer        = &userFile.AllDn;
    bind[16].buffer_length = sizeof(userFile.AllDn);

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
        ZeroMemory(&userFile, sizeof(USERFILE));
        if (mysql_stmt_fetch(stmt) != 0) {
            break;
        }

        // Remove user from the list beforehand in case DbUserReadExtra() fails
        removed = NameListRemove(&list, userName);

        // Read the user's admin-groups, groups, and hosts
        error = DbUserReadExtra(db, userName, &userFile);
        if (error != ERROR_SUCCESS) {
            LOG_WARN("Unable to read user \"%s\" (error %lu).", userName, error);
            continue;
        }

        //
        // If ioFTPD fails to open a user at start-up, the user will still
        // have an entry in the UserIdTable file but ioFTPD considers them
        // gone. The call to UserExists() is done to check for this.
        //
        if (!removed || !UserExists(userName)) {
            TRACE("UserSyncFull: Create(%s)", userName);

            // User does not exist locally, create it.
            error = UserEventCreate(userName, &userFile);
            if (error != ERROR_SUCCESS) {
                LOG_WARN("Unable to create user \"%s\" (error %lu).", userName, error);
            }
        } else {
            TRACE("UserSyncFull: Update(%s)", userName);

            // User already exists locally, update it.
            error = UserEventUpdate(userName, &userFile);
            if (error != ERROR_SUCCESS) {
                LOG_WARN("Unable to update user \"%s\" (error %lu).", userName, error);
            }
        }
    }

    mysql_free_result(metadata);

    //
    // Delete remaining users
    //

    for (i = 0; i < list.count; i++) {
        entry = list.array[i];
        TRACE("UserSyncFull: Delete(%s,%d)", entry->name, entry->id);

        // User does not exist on database, delete it.
        error = UserEventDeleteEx(entry->name, entry->id);
        if (error != ERROR_SUCCESS) {
            LOG_WARN("Unable to delete user \"%s\" (error %lu).", entry->name, error);
        }
    }

    NameListDestroy(&list);

    return ERROR_SUCCESS;
}

static DWORD UserSyncIncrChanges(DB_CONTEXT *db, SYNC_CONTEXT *sync)
{
    CHAR        *query;
    BYTE        syncEvent;
    CHAR        syncInfo[255];
    CHAR        userName[_MAX_NAME + 1];
    DWORD       error;
    USERFILE    userFile;
    INT         result;
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

    query = "SELECT name, type, info FROM io_user_changes"
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
        LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmt));
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

    bindOutput[1].buffer_type   = MYSQL_TYPE_TINY;
    bindOutput[1].buffer        = &syncEvent;
    bindOutput[1].is_unsigned   = TRUE;

    bindOutput[2].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[2].buffer        = syncInfo;
    bindOutput[2].buffer_length = sizeof(syncInfo);

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
                TRACE("UserSyncIncr: Create(%s)", userName);

                // Read user file from database
                ZeroMemory(&userFile, sizeof(USERFILE));
                error = DbUserRead(db, userName, &userFile);
                if (error != ERROR_SUCCESS) {
                    LOG_WARN("Unable to read user \"%s\" (error %lu).", userName, error);
                } else {

                    // Create local user
                    error = UserEventCreate(userName, &userFile);
                    if (error != ERROR_SUCCESS) {
                        LOG_WARN("Unable to create user \"%s\" (error %lu).", userName, error);
                    }
                }
                break;

            case SYNC_EVENT_RENAME:
                TRACE("UserSyncIncr: Rename(%s,%s)", userName, syncInfo);

                error = UserEventRename(userName, syncInfo);
                if (error != ERROR_SUCCESS) {
                    LOG_WARN("Unable to rename user \"%s\" to \"%s\" (error %lu).", userName, syncInfo, error);
                }
                break;

            case SYNC_EVENT_DELETE:
                TRACE("UserSyncIncr: Delete(%s)", userName);

                error = UserEventDelete(userName);
                if (error != ERROR_SUCCESS) {
                    LOG_WARN("Unable to delete user \"%s\" (error %lu).", userName, error);
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

static DWORD UserSyncIncrUpdates(DB_CONTEXT *db, SYNC_CONTEXT *sync)
{
    CHAR        *query;
    CHAR        userName[_MAX_NAME + 1];
    DWORD       error;
    INT         result;
    USERFILE    userFile;
    MYSQL_BIND  bindInput[2];
    MYSQL_BIND  bindOutput[17];
    MYSQL_RES   *metadata;
    MYSQL_STMT  *stmt;

    ASSERT(db != NULL);
    ASSERT(sync != NULL);

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
        LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmt));
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

    bindOutput[1].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[1].buffer        = userFile.Tagline;
    bindOutput[1].buffer_length = sizeof(userFile.Tagline);

    bindOutput[2].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[2].buffer        = userFile.Flags;
    bindOutput[2].buffer_length = sizeof(userFile.Flags);

    bindOutput[3].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[3].buffer        = userFile.Home;
    bindOutput[3].buffer_length = sizeof(userFile.Home);

    bindOutput[4].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[4].buffer        = &userFile.Limits;
    bindOutput[4].buffer_length = sizeof(userFile.Limits);

    bindOutput[5].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[5].buffer        = &userFile.Password;
    bindOutput[5].buffer_length = sizeof(userFile.Password);

    bindOutput[6].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[6].buffer        = userFile.MountFile;
    bindOutput[6].buffer_length = sizeof(userFile.MountFile);

    bindOutput[7].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[7].buffer        = &userFile.Ratio;
    bindOutput[7].buffer_length = sizeof(userFile.Ratio);

    bindOutput[8].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[8].buffer        = &userFile.Credits;
    bindOutput[8].buffer_length = sizeof(userFile.Credits);

    bindOutput[9].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[9].buffer        = &userFile.DayUp;
    bindOutput[9].buffer_length = sizeof(userFile.DayUp);

    bindOutput[10].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[10].buffer        = &userFile.DayDn;
    bindOutput[10].buffer_length = sizeof(userFile.DayDn);

    bindOutput[11].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[11].buffer        = &userFile.WkUp;
    bindOutput[11].buffer_length = sizeof(userFile.WkUp);

    bindOutput[12].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[12].buffer        = &userFile.WkDn;
    bindOutput[12].buffer_length = sizeof(userFile.WkDn);

    bindOutput[13].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[13].buffer        = &userFile.MonthUp;
    bindOutput[13].buffer_length = sizeof(userFile.MonthUp);

    bindOutput[14].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[14].buffer        = &userFile.MonthDn;
    bindOutput[14].buffer_length = sizeof(userFile.MonthDn);

    bindOutput[15].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[15].buffer        = &userFile.AllUp;
    bindOutput[15].buffer_length = sizeof(userFile.AllUp);

    bindOutput[16].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[16].buffer        = &userFile.AllDn;
    bindOutput[16].buffer_length = sizeof(userFile.AllDn);

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
        ZeroMemory(&userFile, sizeof(USERFILE));
        if (mysql_stmt_fetch(stmt) != 0) {
            break;
        }
        TRACE("UserSyncIncr: Update(%s)", userName);

        // Read the user's admin-groups, groups, and hosts
        error = DbUserReadExtra(db, userName, &userFile);
        if (error != ERROR_SUCCESS) {
            LOG_WARN("Unable to read user \"%s\" (error %lu).", userName, error);
        } else {

            // Update user file
            error = UserEventUpdate(userName, &userFile);
            if (error != ERROR_SUCCESS) {
                LOG_WARN("Unable to update user \"%s\" (error %lu).", userName, error);
            }
        }
    }

    mysql_free_result(metadata);

    return ERROR_SUCCESS;
}

static DWORD UserSyncIncr(DB_CONTEXT *db, SYNC_CONTEXT *sync)
{
    DWORD result;

    ASSERT(db != NULL);
    ASSERT(sync != NULL);
    TRACE("db=%p sync=%p", db, sync);

    // Process events from the "io_user_changes" table
    result = UserSyncIncrChanges(db, sync);
    if (result != ERROR_SUCCESS) {
        LOG_ERROR("Unable to sync incremental changes (error %lu).", result);
    }

    // Process updates from the "io_user" table
    result = UserSyncIncrUpdates(db, sync);
    if (result != ERROR_SUCCESS) {
        LOG_ERROR("Unable to sync incremental updates (error %lu).", result);
    }

    return result;
}


DWORD DbUserSync(DB_CONTEXT *db, SYNC_CONTEXT *sync)
{
    DWORD result;

    ASSERT(db != NULL);
    ASSERT(sync != NULL);
    TRACE("db=%p sync=%p", db, sync);

    if (sync->prevUpdate == 0) {
        // If there was no previous update time, we
        // perform a full user syncronization.
        result = UserSyncFull(db);
    } else {
        ASSERT(sync->currUpdate != 0);
        result = UserSyncIncr(db, sync);
    }

    return result;
}
