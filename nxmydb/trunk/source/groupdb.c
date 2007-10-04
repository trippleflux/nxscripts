/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    Group Database Backend

Author:
    neoxed (neoxed@gmail.com) Jun 5, 2006

Abstract:
    Group database storage backend.

*/

#include <base.h>
#include <backends.h>
#include <database.h>

DWORD DbGroupRead(DB_CONTEXT *db, CHAR *groupName, GROUPFILE *groupFilePtr)
{
    CHAR        *query;
    INT         result;
    SIZE_T      groupNameLength;
    ULONG       outputLength;
    GROUPFILE   groupFile;
    MYSQL_BIND  bindInput[1];
    MYSQL_BIND  bindOutput[4];
    MYSQL_RES   *metadata;
    MYSQL_STMT  *stmt;

    ASSERT(db != NULL);
    ASSERT(groupName != NULL);
    ASSERT(groupFilePtr != NULL);
    TRACE("db=%p groupName=%s groupFilePtr=%p", db, groupName, groupFilePtr);

    // Update a local copy of the group-file instead of the actual
    // group-file in case there is an error occurs before the update
    // is completely finished.
    ZeroMemory(&groupFile, sizeof(GROUPFILE));

    stmt = db->stmt[0];

    groupNameLength = strlen(groupName);

    //
    // Prepare statement and bind parameters
    //

    query = "SELECT description, slots, users, vfsfile"
            "  FROM io_group WHERE name=?";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    DB_CHECK_PARAMS(bindInput, stmt);
    ZeroMemory(&bindInput, sizeof(bindInput));

    bindInput[0].buffer_type   = MYSQL_TYPE_STRING;
    bindInput[0].buffer        = groupName;
    bindInput[0].buffer_length = groupNameLength;

    result = mysql_stmt_bind_param(stmt, bindInput);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    metadata = mysql_stmt_result_metadata(stmt);
    if (metadata == NULL) {
        TRACE("Unable to retrieve result metadata: %s", mysql_stmt_error(stmt));
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
    bindOutput[0].buffer        = groupFile.szDescription;
    bindOutput[0].buffer_length = sizeof(groupFile.szDescription);
    bindOutput[0].length        = &outputLength;

    bindOutput[1].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[1].buffer        = groupFile.Slots;
    bindOutput[1].buffer_length = sizeof(groupFile.Slots);
    bindOutput[1].length        = &outputLength;

    bindOutput[2].buffer_type   = MYSQL_TYPE_LONG;
    bindOutput[2].buffer        = &groupFile.Users;

    bindOutput[3].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[3].buffer        = groupFile.szVfsFile;
    bindOutput[3].buffer_length = sizeof(groupFile.szVfsFile);
    bindOutput[3].length        = &outputLength;

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

    result = mysql_stmt_fetch(stmt);
    if (result != 0) {
        TRACE("Unable to fetch results: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    mysql_free_result(metadata);

    //
    // Initialize remaining values of the group-file structure and copy the
    // local group-file to the output parameter. Copy all structure members up
    // to lpInternal, the lpInternal and lpParent members must not be changed.
    //
    groupFile.Gid = groupFilePtr->Gid;

    CopyMemory(groupFilePtr, &groupFile, offsetof(GROUPFILE, lpInternal));

    return ERROR_SUCCESS;
}

DWORD DbGroupCreate(DB_CONTEXT *db, CHAR *groupName, GROUPFILE *groupFile)
{
    BYTE        syncEvent;
    CHAR        *query;
    DWORD       error;
    INT         result;
    SIZE_T      groupNameLength;
    MYSQL_BIND  bindChanges[2];
    MYSQL_BIND  bindGroup[5];
    MYSQL_STMT  *stmtChanges;
    MYSQL_STMT  *stmtGroup;

    ASSERT(db != NULL);
    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);
    TRACE("db=%p groupName=%s groupFile=%p", db, groupName, groupFile);

    stmtGroup   = db->stmt[0];
    stmtChanges = db->stmt[1];

    groupNameLength = strlen(groupName);

    //
    // Prepare group statement and bind parameters
    //

    query = "INSERT INTO io_group"
            " (name,description,slots,users,vfsfile)"
            " VALUES(?,?,?,?,?)";

    result = mysql_stmt_prepare(stmtGroup, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s", mysql_stmt_error(stmtGroup));
        return DbMapErrorFromStmt(stmtGroup);
    }

    DB_CHECK_PARAMS(bindGroup, stmtGroup);
    ZeroMemory(&bindGroup, sizeof(bindGroup));

    bindGroup[0].buffer_type   = MYSQL_TYPE_STRING;
    bindGroup[0].buffer        = groupName;
    bindGroup[0].buffer_length = groupNameLength;

    bindGroup[1].buffer_type   = MYSQL_TYPE_STRING;
    bindGroup[1].buffer        = groupFile->szDescription;
    bindGroup[1].buffer_length = sizeof(groupFile->szDescription) - 1;

    bindGroup[2].buffer_type   = MYSQL_TYPE_BLOB;
    bindGroup[2].buffer        = groupFile->Slots;
    bindGroup[2].buffer_length = sizeof(groupFile->Slots);

    bindGroup[3].buffer_type   = MYSQL_TYPE_LONG;
    bindGroup[3].buffer        = &groupFile->Users;

    bindGroup[4].buffer_type   = MYSQL_TYPE_STRING;
    bindGroup[4].buffer        = groupFile->szVfsFile;
    bindGroup[4].buffer_length = sizeof(groupFile->szVfsFile) - 1;

    result = mysql_stmt_bind_param(stmtGroup, bindGroup);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s", mysql_stmt_error(stmtGroup));
        return DbMapErrorFromStmt(stmtGroup);
    }

    //
    // Prepare changes statement and bind parameters
    //

    query = "INSERT INTO io_group_changes"
            " (time,type,name)"
            " VALUES(UNIX_TIMESTAMP(),?,?)";

    result = mysql_stmt_prepare(stmtChanges, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s", mysql_stmt_error(stmtChanges));
        return DbMapErrorFromStmt(stmtChanges);
    }

    DB_CHECK_PARAMS(bindChanges, stmtChanges);
    ZeroMemory(&bindChanges, sizeof(bindChanges));

    // Change event used during incremental syncs.
    syncEvent = SYNC_EVENT_CREATE;

    bindChanges[0].buffer_type   = MYSQL_TYPE_TINY;
    bindChanges[0].buffer        = &syncEvent;
    bindChanges[0].is_unsigned   = TRUE;

    bindChanges[1].buffer_type   = MYSQL_TYPE_STRING;
    bindChanges[1].buffer        = groupName;
    bindChanges[1].buffer_length = groupNameLength;

    result = mysql_stmt_bind_param(stmtChanges, bindChanges);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s", mysql_stmt_error(stmtChanges));
        return DbMapErrorFromStmt(stmtChanges);
    }

    //
    // Begin transaction
    //

    result = mysql_query(db->handle, "START TRANSACTION");
    if (result != 0) {
        TRACE("Unable to start transaction: %s", mysql_error(db->handle));
        return DbMapErrorFromConn(db->handle);
    }

    //
    // Execute prepared statements
    //

    result = mysql_stmt_execute(stmtGroup);
    if (result != 0) {
        TRACE("Unable to execute statement: %s", mysql_stmt_error(stmtGroup));
        error = DbMapErrorFromStmt(stmtGroup);
        goto rollback;
    }

    result = mysql_stmt_execute(stmtChanges);
    if (result != 0) {
        TRACE("Unable to execute statement: %s", mysql_stmt_error(stmtChanges));
        error = DbMapErrorFromStmt(stmtChanges);
        goto rollback;
    }

    //
    // Commit transaction
    //

    result = mysql_query(db->handle, "COMMIT");
    if (result != 0) {
        TRACE("Unable to commit transaction: %s", mysql_error(db->handle));
        return DbMapErrorFromConn(db->handle);
    }

    return ERROR_SUCCESS;

rollback:
    //
    // Rollback transaction on error
    //

    if (mysql_query(db->handle, "ROLLBACK") != 0) {
        TRACE("Unable to rollback transaction: %s", mysql_error(db->handle));
    }

    ASSERT(error != ERROR_SUCCESS);
    return error;
}

DWORD DbGroupRename(DB_CONTEXT *db, CHAR *groupName, CHAR *newName)
{
    ASSERT(db != NULL);
    ASSERT(groupName != NULL);
    ASSERT(newName != NULL);
    TRACE("db=%p groupName=%s newName=%s", db, groupName, newName);

    // TODO

    return ERROR_NOT_SUPPORTED;
}

DWORD DbGroupDelete(DB_CONTEXT *db, CHAR *groupName)
{
    BYTE        syncEvent;
    CHAR        *query;
    DWORD       error;
    INT         result;
    INT64       affectedRows;
    SIZE_T      groupNameLength;
    MYSQL_BIND  bindChanges[2];
    MYSQL_BIND  bindGroup[1];
    MYSQL_STMT  *stmtChanges;
    MYSQL_STMT  *stmtGroup;

    ASSERT(db != NULL);
    ASSERT(groupName != NULL);
    TRACE("db=%p groupName=%s", db, groupName);

    stmtGroup   = db->stmt[0];
    stmtChanges = db->stmt[1];

    groupNameLength = strlen(groupName);

    //
    // Prepare group statement and bind parameters
    //

    query = "DELETE FROM io_group WHERE name=?";

    result = mysql_stmt_prepare(stmtGroup, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s", mysql_stmt_error(stmtGroup));
        return DbMapErrorFromStmt(stmtGroup);
    }

    DB_CHECK_PARAMS(bindGroup, stmtGroup);
    ZeroMemory(&bindGroup, sizeof(bindGroup));

    bindGroup[0].buffer_type   = MYSQL_TYPE_STRING;
    bindGroup[0].buffer        = groupName;
    bindGroup[0].buffer_length = groupNameLength;

    result = mysql_stmt_bind_param(stmtGroup, bindGroup);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s", mysql_stmt_error(stmtGroup));
        return DbMapErrorFromStmt(stmtGroup);
    }

    //
    // Prepare changes statement and bind parameters
    //

    query = "INSERT INTO io_group_changes"
            " (time,type,name)"
            " VALUES(UNIX_TIMESTAMP(),?,?)";

    result = mysql_stmt_prepare(stmtChanges, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s", mysql_stmt_error(stmtChanges));
        return DbMapErrorFromStmt(stmtChanges);
    }

    DB_CHECK_PARAMS(bindChanges, stmtChanges);
    ZeroMemory(&bindChanges, sizeof(bindChanges));

    // Change event used during incremental syncs.
    syncEvent = SYNC_EVENT_DELETE;

    bindChanges[0].buffer_type   = MYSQL_TYPE_TINY;
    bindChanges[0].buffer        = &syncEvent;
    bindChanges[0].is_unsigned   = TRUE;

    bindChanges[1].buffer_type   = MYSQL_TYPE_STRING;
    bindChanges[1].buffer        = groupName;
    bindChanges[1].buffer_length = groupNameLength;

    result = mysql_stmt_bind_param(stmtChanges, bindChanges);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s", mysql_stmt_error(stmtChanges));
        return DbMapErrorFromStmt(stmtChanges);
    }

    //
    // Begin transaction
    //

    result = mysql_query(db->handle, "START TRANSACTION");
    if (result != 0) {
        TRACE("Unable to start transaction: %s", mysql_error(db->handle));
        return DbMapErrorFromConn(db->handle);
    }

    //
    // Execute prepared statements
    //

    result = mysql_stmt_execute(stmtGroup);
    if (result != 0) {
        TRACE("Unable to execute statement: %s", mysql_stmt_error(stmtGroup));
        error = DbMapErrorFromStmt(stmtGroup);
        goto rollback;
    }

    affectedRows = mysql_stmt_affected_rows(stmtGroup);
    if (affectedRows > 0) {

        // Only insert a changes event if the group was deleted.
        result = mysql_stmt_execute(stmtChanges);
        if (result != 0) {
            TRACE("Unable to execute statement: %s", mysql_stmt_error(stmtChanges));
            error = DbMapErrorFromStmt(stmtChanges);
            goto rollback;
        }
    }

    //
    // Commit transaction
    //

    result = mysql_query(db->handle, "COMMIT");
    if (result != 0) {
        TRACE("Unable to commit transaction: %s", mysql_error(db->handle));
        return DbMapErrorFromConn(db->handle);
    }

    //
    // Check for deleted rows
    //

    if (affectedRows == 0) {
        TRACE("Unable to delete group (no affected rows).");
        return ERROR_GROUP_NOT_FOUND;
    }

    return ERROR_SUCCESS;

rollback:
    //
    // Rollback transaction on error
    //

    if (mysql_query(db->handle, "ROLLBACK") != 0) {
        TRACE("Unable to rollback transaction: %s", mysql_error(db->handle));
    }

    ASSERT(error != ERROR_SUCCESS);
    return error;
}

DWORD DbGroupLock(DB_CONTEXT *db, CHAR *groupName, GROUPFILE *groupFile)
{
    CHAR        *query;
    DWORD       error;
    INT         result;
    INT64       affectedRows;
    MYSQL_BIND  bind[3];
    MYSQL_STMT  *stmt;

    ASSERT(db != NULL);
    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);
    TRACE("db=%p groupName=%s groupFile=%p", db, groupName, groupFile);

    stmt = db->stmt[0];

    //
    // Prepare statement and bind parameters
    //

    // Parameters: group, expire, timeout, owner
    query = "CALL io_group_lock(?,?,?,?)";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    DB_CHECK_PARAMS(bind, stmt);
    ZeroMemory(&bind, sizeof(bind));

    bind[0].buffer_type   = MYSQL_TYPE_STRING;
    bind[0].buffer        = groupName;
    bind[0].buffer_length = strlen(groupName);

    bind[1].buffer_type   = MYSQL_TYPE_LONG;
    bind[1].buffer        = &dbConfigLock.expire;
    bind[1].is_unsigned   = TRUE;

    bind[2].buffer_type   = MYSQL_TYPE_LONG;
    bind[2].buffer        = &dbConfigLock.timeout;
    bind[2].is_unsigned   = TRUE;

    bind[3].buffer_type   = MYSQL_TYPE_STRING;
    bind[3].buffer        = dbConfigLock.owner;
    bind[3].buffer_length = dbConfigLock.ownerLength;

    result = mysql_stmt_bind_param(stmt, bind);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s", mysql_stmt_error(stmt));
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
    // Check for modified rows
    //

    affectedRows = mysql_stmt_affected_rows(stmt);
    if (affectedRows == 0) {
        TRACE("Unable to lock group (no affected rows).");
        return ERROR_GROUP_LOCK_FAILED;
    }

    ASSERT(affectedRows == 1);

    //
    // Update group data
    //

    error = DbGroupRead(db, groupName, groupFile);
    if (error != ERROR_SUCCESS) {
        LOG_WARN("Unable to update group on lock (error %lu).", error);
    }

    return error;
}

DWORD DbGroupUnlock(DB_CONTEXT *db, CHAR *groupName)
{
    CHAR        *query;
    INT         result;
    INT64       affectedRows;
    MYSQL_BIND  bind[2];
    MYSQL_STMT  *stmt;

    ASSERT(db != NULL);
    ASSERT(groupName != NULL);
    TRACE("db=%p groupName=%s", db, groupName);

    stmt = db->stmt[0];

    //
    // Prepare statement and bind parameters
    //

    query = "UPDATE io_group SET lockowner=NULL, locktime=0"
            "  WHERE name=? AND lockowner=?";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    DB_CHECK_PARAMS(bind, stmt);
    ZeroMemory(&bind, sizeof(bind));

    bind[0].buffer_type   = MYSQL_TYPE_STRING;
    bind[0].buffer        = groupName;
    bind[0].buffer_length = strlen(groupName);

    bind[1].buffer_type   = MYSQL_TYPE_STRING;
    bind[1].buffer        = dbConfigLock.owner;
    bind[1].buffer_length = dbConfigLock.ownerLength;

    result = mysql_stmt_bind_param(stmt, bind);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s", mysql_stmt_error(stmt));
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

    affectedRows = mysql_stmt_affected_rows(stmt);
    if (affectedRows == 0) {
        // Failure is acceptable
        TRACE("Unable to unlock group (no affected rows).");
    }

    return ERROR_SUCCESS;
}

DWORD DbGroupOpen(DB_CONTEXT *db, CHAR *groupName, GROUPFILE *groupFile)
{
    ASSERT(db != NULL);
    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);
    TRACE("db=%p groupName=%s groupFile=%p", db, groupName, groupFile);

    return DbGroupRead(db, groupName, groupFile);
}

DWORD DbGroupWrite(DB_CONTEXT *db, CHAR *groupName, GROUPFILE *groupFile)
{
    CHAR        *query;
    INT         result;
    SIZE_T      groupNameLength;
    MYSQL_BIND  bind[5];
    MYSQL_STMT  *stmt;

    ASSERT(db != NULL);
    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);
    TRACE("db=%p groupName=%s groupFile=%p", db, groupName, groupFile);

    stmt = db->stmt[0];

    groupNameLength = strlen(groupName);

    //
    // Prepare statement and bind parameters
    //

    query = "UPDATE io_group SET description=?, slots=?,"
            " users=?, vfsfile=?, updated=UNIX_TIMESTAMP()"
            "   WHERE name=?";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    DB_CHECK_PARAMS(bind, stmt);
    ZeroMemory(&bind, sizeof(bind));

    bind[0].buffer_type   = MYSQL_TYPE_STRING;
    bind[0].buffer        = groupFile->szDescription;
    bind[0].buffer_length = sizeof(groupFile->szDescription) - 1;

    bind[1].buffer_type   = MYSQL_TYPE_BLOB;
    bind[1].buffer        = groupFile->Slots;
    bind[1].buffer_length = sizeof(groupFile->Slots);

    bind[2].buffer_type   = MYSQL_TYPE_LONG;
    bind[2].buffer        = &groupFile->Users;

    bind[3].buffer_type   = MYSQL_TYPE_STRING;
    bind[3].buffer        = groupFile->szVfsFile;
    bind[3].buffer_length = sizeof(groupFile->szVfsFile) - 1;

    bind[4].buffer_type   = MYSQL_TYPE_STRING;
    bind[4].buffer        = groupName;
    bind[4].buffer_length = groupNameLength;

    result = mysql_stmt_bind_param(stmt, bind);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s", mysql_stmt_error(stmt));
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

    return ERROR_SUCCESS;
}

DWORD DbGroupClose(GROUPFILE *groupFile)
{
    UNREFERENCED_PARAMETER(groupFile);
    return ERROR_SUCCESS;
}
