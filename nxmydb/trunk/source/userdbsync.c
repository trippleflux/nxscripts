/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2009 neoxed

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
        LOG_ERROR("Unable to allocate memory for module context.");

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
            if (result == ERROR_SUCCESS) {
                // When ioFTPD creates a new user, it calls the module's exported
                // Create() function and then it calls Write(). When a module
                // creates a new user, it's the module's responsibility to update
                // the new user file.
                //
                // Success of FileUserWrite() does not matter.
                FileUserWrite(userFile);
            } else {
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
        TRACE("Unable to delete user file for \"%s\" (error %lu).", userName, result);
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

    // Update ioFTPD's user file structure
    result = UserUpdateByName(userName, userFile);

    if (result == ERROR_SUCCESS) {
        // ioFTPD sets the lpInternal field if the update was successful
        ASSERT(userFile->lpInternal != NULL);

        // Update the user file
        FileUserWrite(userFile);
    }

    return result;
}


static DWORD UserSyncFull(DB_CONTEXT *db)
{
    BOOL        removed;
    CHAR        *query;
    CHAR        deletedBy[_MAX_NAME + 1];
    CHAR        userName[_MAX_NAME + 1];
    DWORD       error;
    DWORD       i;
    USERFILE    userFile;
    INT         result;
    NAME_ENTRY  *entry;
    NAME_LIST   list;
    MYSQL_BIND  bind[31];
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
            "       ratio,alldn,allup,daydn,dayup,monthdn,monthup,wkdn,wkup,"
            "       creator,createdon,logoncount,logonlast,logonhost,maxups,"
            "       maxdowns,maxlogins,expiresat,deletedon,deletedby,"
            "       deletedmsg,theme,opaque"
            "  FROM io_user";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    metadata = mysql_stmt_result_metadata(stmt);
    if (metadata == NULL) {
        LOG_WARN("Unable to retrieve result metadata: %s", mysql_stmt_error(stmt));
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

    // SELECT name
    bind[0].buffer_type   = MYSQL_TYPE_STRING;
    bind[0].buffer        = userName;
    bind[0].buffer_length = sizeof(userName);

    // SELECT description
    bind[1].buffer_type   = MYSQL_TYPE_STRING;
    bind[1].buffer        = userFile.Tagline;
    bind[1].buffer_length = sizeof(userFile.Tagline);

    // SELECT flags
    bind[2].buffer_type   = MYSQL_TYPE_STRING;
    bind[2].buffer        = userFile.Flags;
    bind[2].buffer_length = sizeof(userFile.Flags);

    // SELECT home
    bind[3].buffer_type   = MYSQL_TYPE_STRING;
    bind[3].buffer        = userFile.Home;
    bind[3].buffer_length = sizeof(userFile.Home);

    // SELECT limits
    bind[4].buffer_type   = MYSQL_TYPE_BLOB;
    bind[4].buffer        = &userFile.Limits;
    bind[4].buffer_length = sizeof(userFile.Limits);

    // SELECT password
    bind[5].buffer_type   = MYSQL_TYPE_BLOB;
    bind[5].buffer        = &userFile.Password;
    bind[5].buffer_length = sizeof(userFile.Password);

    // SELECT vfsfile
    bind[6].buffer_type   = MYSQL_TYPE_STRING;
    bind[6].buffer        = userFile.MountFile;
    bind[6].buffer_length = sizeof(userFile.MountFile);

    // SELECT credits
    bind[7].buffer_type   = MYSQL_TYPE_BLOB;
    bind[7].buffer        = &userFile.Credits;
    bind[7].buffer_length = sizeof(userFile.Credits);

    // SELECT ratio
    bind[8].buffer_type   = MYSQL_TYPE_BLOB;
    bind[8].buffer        = &userFile.Ratio;
    bind[8].buffer_length = sizeof(userFile.Ratio);

    // SELECT alldn
    bind[9].buffer_type   = MYSQL_TYPE_BLOB;
    bind[9].buffer        = &userFile.AllDn;
    bind[9].buffer_length = sizeof(userFile.AllDn);

    // SELECT allup
    bind[10].buffer_type   = MYSQL_TYPE_BLOB;
    bind[10].buffer        = &userFile.AllUp;
    bind[10].buffer_length = sizeof(userFile.AllUp);

    // SELECT daydn
    bind[11].buffer_type   = MYSQL_TYPE_BLOB;
    bind[11].buffer        = &userFile.DayDn;
    bind[11].buffer_length = sizeof(userFile.DayDn);

    // SELECT dayup
    bind[12].buffer_type   = MYSQL_TYPE_BLOB;
    bind[12].buffer        = &userFile.DayUp;
    bind[12].buffer_length = sizeof(userFile.DayUp);

    // SELECT monthdn
    bind[13].buffer_type   = MYSQL_TYPE_BLOB;
    bind[13].buffer        = &userFile.MonthDn;
    bind[13].buffer_length = sizeof(userFile.MonthDn);

    // SELECT monthup
    bind[14].buffer_type   = MYSQL_TYPE_BLOB;
    bind[14].buffer        = &userFile.MonthUp;
    bind[14].buffer_length = sizeof(userFile.MonthUp);

    // SELECT wkdn
    bind[15].buffer_type   = MYSQL_TYPE_BLOB;
    bind[15].buffer        = &userFile.WkUp;
    bind[15].buffer_length = sizeof(userFile.WkUp);

    // SELECT wkup
    bind[16].buffer_type   = MYSQL_TYPE_BLOB;
    bind[16].buffer        = &userFile.WkDn;
    bind[16].buffer_length = sizeof(userFile.WkDn);

    // SELECT creator
    bind[17].buffer_type   = MYSQL_TYPE_STRING;
    bind[17].buffer        = &userFile.CreatorName;
    bind[17].buffer_length = sizeof(userFile.CreatorName);

    // SELECT createdon
    bind[18].buffer_type   = MYSQL_TYPE_LONGLONG;
    bind[18].buffer        = &userFile.CreatedOn;

    // SELECT logoncount
    bind[19].buffer_type   = MYSQL_TYPE_LONG;
    bind[19].buffer        = &userFile.LogonCount;

    // SELECT logonlast
    bind[20].buffer_type   = MYSQL_TYPE_LONGLONG;
    bind[20].buffer        = &userFile.LogonLast;

    // SELECT logonhost
    bind[21].buffer_type   = MYSQL_TYPE_STRING;
    bind[21].buffer        = &userFile.LogonHost;
    bind[21].buffer_length = sizeof(userFile.LogonHost);

    // SELECT maxups
    bind[22].buffer_type   = MYSQL_TYPE_LONG;
    bind[22].buffer        = &userFile.MaxUploads;

    // SELECT maxdowns
    bind[23].buffer_type   = MYSQL_TYPE_LONG;
    bind[23].buffer        = &userFile.MaxDownloads;

    // SELECT maxlogins
    bind[24].buffer_type   = MYSQL_TYPE_LONG;
    bind[24].buffer        = &userFile.LimitPerIP;

    // SELECT expiresat
    bind[25].buffer_type   = MYSQL_TYPE_LONGLONG;
    bind[25].buffer        = &userFile.ExpiresAt;

    // SELECT deletedon
    bind[26].buffer_type   = MYSQL_TYPE_LONGLONG;
    bind[26].buffer        = &userFile.DeletedOn;

    // SELECT deletedby
    bind[27].buffer_type   = MYSQL_TYPE_STRING;
    bind[27].buffer        = &deletedBy;
    bind[27].buffer_length = sizeof(deletedBy);

    // SELECT deletedmsg
    bind[28].buffer_type   = MYSQL_TYPE_STRING;
    bind[28].buffer        = &userFile.DeletedMsg;
    bind[28].buffer_length = sizeof(userFile.DeletedMsg);

    // SELECT theme
    bind[29].buffer_type   = MYSQL_TYPE_LONG;
    bind[29].buffer        = &userFile.Theme;

    // SELECT opaque
    bind[30].buffer_type   = MYSQL_TYPE_STRING;
    bind[30].buffer        = &userFile.Opaque;
    bind[30].buffer_length = sizeof(userFile.Opaque);

    result = mysql_stmt_bind_result(stmt, bind);
    if (result != 0) {
        LOG_WARN("Unable to bind results: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    result = mysql_stmt_store_result(stmt);
    if (result != 0) {
        LOG_WARN("Unable to buffer results: %s", mysql_stmt_error(stmt));
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

        // Initialize remaining values of the user-file structure.
        userFile.Gid        = userFile.Groups[0];
        userFile.CreatorUid = Io_User2Uid(userFile.CreatorName);
        userFile.DeletedBy  = Io_User2Uid(deletedBy);

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

static DWORD UserSyncIncrChanges(DB_CONTEXT *db, DB_SYNC *sync)
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
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    DB_CHECK_PARAMS(bindInput, stmt);
    ZeroMemory(&bindInput, sizeof(bindInput));

    // BETWEEN ?
    bindInput[0].buffer_type = MYSQL_TYPE_LONG;
    bindInput[0].buffer      = &sync->prevUpdate;
    bindInput[0].is_unsigned = TRUE;

    // AND ?
    bindInput[1].buffer_type = MYSQL_TYPE_LONG;
    bindInput[1].buffer      = &sync->currUpdate;
    bindInput[1].is_unsigned = TRUE;

    result = mysql_stmt_bind_param(stmt, bindInput);
    if (result != 0) {
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    metadata = mysql_stmt_result_metadata(stmt);
    if (metadata == NULL) {
        LOG_WARN("Unable to retrieve result metadata: %s", mysql_stmt_error(stmt));
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

    // SELECT name
    bindOutput[0].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[0].buffer        = userName;
    bindOutput[0].buffer_length = sizeof(userName);

    // SELECT type
    bindOutput[1].buffer_type   = MYSQL_TYPE_TINY;
    bindOutput[1].buffer        = &syncEvent;
    bindOutput[1].is_unsigned   = TRUE;

    // SELECT info
    bindOutput[2].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[2].buffer        = syncInfo;
    bindOutput[2].buffer_length = sizeof(syncInfo);

    result = mysql_stmt_bind_result(stmt, bindOutput);
    if (result != 0) {
        LOG_WARN("Unable to bind results: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    result = mysql_stmt_store_result(stmt);
    if (result != 0) {
        LOG_WARN("Unable to buffer results: %s", mysql_stmt_error(stmt));
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

static DWORD UserSyncIncrUpdates(DB_CONTEXT *db, DB_SYNC *sync)
{
    CHAR        *query;
    CHAR        deletedBy[_MAX_NAME + 1];
    CHAR        userName[_MAX_NAME + 1];
    DWORD       error;
    INT         result;
    USERFILE    userFile;
    MYSQL_BIND  bindInput[2];
    MYSQL_BIND  bindOutput[31];
    MYSQL_RES   *metadata;
    MYSQL_STMT  *stmt;

    ASSERT(db != NULL);
    ASSERT(sync != NULL);

    //
    // Prepare statement and bind parameters
    //

    stmt = db->stmt[7];

    query = "SELECT name,description,flags,home,limits,password,vfsfile,credits,"
            "       ratio,alldn,allup,daydn,dayup,monthdn,monthup,wkdn,wkup,"
            "       creator,createdon,logoncount,logonlast,logonhost,maxups,"
            "       maxdowns,maxlogins,expiresat,deletedon,deletedby,"
            "       deletedmsg,theme,opaque"
            "  FROM io_user"
            "  WHERE updated BETWEEN ? AND ?";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    DB_CHECK_PARAMS(bindInput, stmt);
    ZeroMemory(&bindInput, sizeof(bindInput));

    // BETWEEN ?
    bindInput[0].buffer_type = MYSQL_TYPE_LONG;
    bindInput[0].buffer      = &sync->prevUpdate;
    bindInput[0].is_unsigned = TRUE;

    // AND ?
    bindInput[1].buffer_type = MYSQL_TYPE_LONG;
    bindInput[1].buffer      = &sync->currUpdate;
    bindInput[1].is_unsigned = TRUE;

    result = mysql_stmt_bind_param(stmt, bindInput);
    if (result != 0) {
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    metadata = mysql_stmt_result_metadata(stmt);
    if (metadata == NULL) {
        LOG_WARN("Unable to retrieve result metadata: %s", mysql_stmt_error(stmt));
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

    // SELECT name
    bindOutput[0].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[0].buffer        = userName;
    bindOutput[0].buffer_length = sizeof(userName);

    // SELECT description
    bindOutput[1].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[1].buffer        = userFile.Tagline;
    bindOutput[1].buffer_length = sizeof(userFile.Tagline);

    // SELECT flags
    bindOutput[2].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[2].buffer        = userFile.Flags;
    bindOutput[2].buffer_length = sizeof(userFile.Flags);

    // SELECT home
    bindOutput[3].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[3].buffer        = userFile.Home;
    bindOutput[3].buffer_length = sizeof(userFile.Home);

    // SELECT limits
    bindOutput[4].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[4].buffer        = &userFile.Limits;
    bindOutput[4].buffer_length = sizeof(userFile.Limits);

    // SELECT password
    bindOutput[5].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[5].buffer        = &userFile.Password;
    bindOutput[5].buffer_length = sizeof(userFile.Password);

    // SELECT vfsfile
    bindOutput[6].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[6].buffer        = userFile.MountFile;
    bindOutput[6].buffer_length = sizeof(userFile.MountFile);

    // SELECT credits
    bindOutput[7].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[7].buffer        = &userFile.Credits;
    bindOutput[7].buffer_length = sizeof(userFile.Credits);

    // SELECT ratio
    bindOutput[8].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[8].buffer        = &userFile.Ratio;
    bindOutput[8].buffer_length = sizeof(userFile.Ratio);

    // SELECT alldn
    bindOutput[9].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[9].buffer        = &userFile.AllDn;
    bindOutput[9].buffer_length = sizeof(userFile.AllDn);

    // SELECT allup
    bindOutput[10].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[10].buffer        = &userFile.AllUp;
    bindOutput[10].buffer_length = sizeof(userFile.AllUp);

    // SELECT daydn
    bindOutput[11].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[11].buffer        = &userFile.DayDn;
    bindOutput[11].buffer_length = sizeof(userFile.DayDn);

    // SELECT dayup
    bindOutput[12].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[12].buffer        = &userFile.DayUp;
    bindOutput[12].buffer_length = sizeof(userFile.DayUp);

    // SELECT monthdn
    bindOutput[13].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[13].buffer        = &userFile.MonthDn;
    bindOutput[13].buffer_length = sizeof(userFile.MonthDn);

    // SELECT monthup
    bindOutput[14].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[14].buffer        = &userFile.MonthUp;
    bindOutput[14].buffer_length = sizeof(userFile.MonthUp);

    // SELECT wkdn
    bindOutput[15].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[15].buffer        = &userFile.WkUp;
    bindOutput[15].buffer_length = sizeof(userFile.WkUp);

    // SELECT wkup
    bindOutput[16].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[16].buffer        = &userFile.WkDn;
    bindOutput[16].buffer_length = sizeof(userFile.WkDn);

    // SELECT creator
    bindOutput[17].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[17].buffer        = &userFile.CreatorName;
    bindOutput[17].buffer_length = sizeof(userFile.CreatorName);

    // SELECT createdon
    bindOutput[18].buffer_type   = MYSQL_TYPE_LONGLONG;
    bindOutput[18].buffer        = &userFile.CreatedOn;

    // SELECT logoncount
    bindOutput[19].buffer_type   = MYSQL_TYPE_LONG;
    bindOutput[19].buffer        = &userFile.LogonCount;

    // SELECT logonlast
    bindOutput[20].buffer_type   = MYSQL_TYPE_LONGLONG;
    bindOutput[20].buffer        = &userFile.LogonLast;

    // SELECT logonhost
    bindOutput[21].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[21].buffer        = &userFile.LogonHost;
    bindOutput[21].buffer_length = sizeof(userFile.LogonHost);

    // SELECT maxups
    bindOutput[22].buffer_type   = MYSQL_TYPE_LONG;
    bindOutput[22].buffer        = &userFile.MaxUploads;

    // SELECT maxdowns
    bindOutput[23].buffer_type   = MYSQL_TYPE_LONG;
    bindOutput[23].buffer        = &userFile.MaxDownloads;

    // SELECT maxlogins
    bindOutput[24].buffer_type   = MYSQL_TYPE_LONG;
    bindOutput[24].buffer        = &userFile.LimitPerIP;

    // SELECT expiresat
    bindOutput[25].buffer_type   = MYSQL_TYPE_LONGLONG;
    bindOutput[25].buffer        = &userFile.ExpiresAt;

    // SELECT deletedon
    bindOutput[26].buffer_type   = MYSQL_TYPE_LONGLONG;
    bindOutput[26].buffer        = &userFile.DeletedOn;

    // SELECT deletedby
    bindOutput[27].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[27].buffer        = &deletedBy;
    bindOutput[27].buffer_length = sizeof(deletedBy);

    // SELECT deletedmsg
    bindOutput[28].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[28].buffer        = &userFile.DeletedMsg;
    bindOutput[28].buffer_length = sizeof(userFile.DeletedMsg);

    // SELECT theme
    bindOutput[29].buffer_type   = MYSQL_TYPE_LONG;
    bindOutput[29].buffer        = &userFile.Theme;

    // SELECT opaque
    bindOutput[30].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[30].buffer        = &userFile.Opaque;
    bindOutput[30].buffer_length = sizeof(userFile.Opaque);

    result = mysql_stmt_bind_result(stmt, bindOutput);
    if (result != 0) {
        LOG_WARN("Unable to bind results: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    result = mysql_stmt_store_result(stmt);
    if (result != 0) {
        LOG_WARN("Unable to buffer results: %s", mysql_stmt_error(stmt));
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
            // Initialize remaining values of the user-file structure.
            userFile.Gid        = userFile.Groups[0];
            userFile.CreatorUid = Io_User2Uid(userFile.CreatorName);
            userFile.DeletedBy  = Io_User2Uid(deletedBy);

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

static DWORD UserSyncIncr(DB_CONTEXT *db, DB_SYNC *sync)
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


DWORD DbUserPurge(DB_CONTEXT *db, INT age)
{
    CHAR        *query;
    INT         result;
    MYSQL_BIND  bind[1];
    MYSQL_STMT  *stmt;

    ASSERT(db != NULL);
    TRACE("db=%p age=%d", db, age);

    stmt = db->stmt[0];

    //
    // Prepare statement and bind parameters
    //

    query = "DELETE FROM io_user_changes WHERE time < (UNIX_TIMESTAMP() - ?)";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    DB_CHECK_PARAMS(bind, stmt);
    ZeroMemory(&bind, sizeof(bind));

    // UNIX_TIMESTAMP() - ?
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer      = &age;

    result = mysql_stmt_bind_param(stmt, bind);
    if (result != 0) {
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmt));
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

    TRACE("Purged %I64u entries from the user changes table.", mysql_stmt_affected_rows(stmt));

    return ERROR_SUCCESS;
}

DWORD DbUserSync(DB_CONTEXT *db, DB_SYNC *sync)
{
    DWORD result;

    ASSERT(db != NULL);
    ASSERT(sync != NULL);
    TRACE("db=%p sync=%p", db, sync);

    if (sync->prevUpdate == 0) {
        // If there was no previous update time, we perform a full user synchronization.
        result = UserSyncFull(db);
    } else {
        ASSERT(sync->currUpdate != 0);
        result = UserSyncIncr(db, sync);
    }

    return result;
}
