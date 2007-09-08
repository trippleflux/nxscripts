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

static DWORD DbGroupRead(DB_CONTEXT *db, CHAR *groupName, GROUPFILE *groupFilePtr)
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
    TRACE("db=%p groupName=%s groupFilePtr=%p\n", db, groupName, groupFilePtr);

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
            "  FROM io_groups WHERE name=?";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    DB_CHECK_PARAMS(bindInput, stmt);
    ZeroMemory(&bindInput, sizeof(bindInput));

    bindInput[0].buffer_type   = MYSQL_TYPE_STRING;
    bindInput[0].buffer        = groupName;
    bindInput[0].buffer_length = groupNameLength;

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
        TRACE("Unable to bind results: %s\n", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    result = mysql_stmt_fetch(stmt);
    if (result != 0) {
        TRACE("Unable to fetch results: %s\n", mysql_stmt_error(stmt));
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
    CHAR        *query;
    INT         result;
    SIZE_T      groupNameLength;
    MYSQL_BIND  bind[5];
    MYSQL_STMT  *stmt;

    ASSERT(db != NULL);
    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);
    TRACE("db=%p groupName=%s groupFile=%p\n", db, groupName, groupFile);

    stmt = db->stmt[0];

    groupNameLength = strlen(groupName);

    //
    // Prepare statement and bind parameters
    //

    query = "INSERT INTO io_groups"
            " (name,description,slots,users,vfsfile,updated)"
            " VALUES(?,?,?,?,?,UNIX_TIMESTAMP())";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    DB_CHECK_PARAMS(bind, stmt);
    ZeroMemory(&bind, sizeof(bind));

    bind[0].buffer_type   = MYSQL_TYPE_STRING;
    bind[0].buffer        = groupName;
    bind[0].buffer_length = groupNameLength;

    bind[1].buffer_type   = MYSQL_TYPE_STRING;
    bind[1].buffer        = groupFile->szDescription;
    bind[1].buffer_length = sizeof(groupFile->szDescription) - 1;

    bind[2].buffer_type   = MYSQL_TYPE_BLOB;
    bind[2].buffer        = groupFile->Slots;
    bind[2].buffer_length = sizeof(groupFile->Slots);

    bind[3].buffer_type   = MYSQL_TYPE_LONG;
    bind[3].buffer        = &groupFile->Users;

    bind[4].buffer_type   = MYSQL_TYPE_STRING;
    bind[4].buffer        = groupFile->szVfsFile;
    bind[4].buffer_length = sizeof(groupFile->szVfsFile) - 1;

    result = mysql_stmt_bind_param(stmt, bind);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmt));
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

    return ERROR_SUCCESS;
}

DWORD DbGroupRename(DB_CONTEXT *db, CHAR *groupName, CHAR *newName)
{
    ASSERT(db != NULL);
    ASSERT(groupName != NULL);
    ASSERT(newName != NULL);
    TRACE("db=%p groupName=%s newName=%s\n", db, groupName, newName);

    // TODO

    return ERROR_NOT_SUPPORTED;
}

DWORD DbGroupDelete(DB_CONTEXT *db, CHAR *groupName)
{
    CHAR        *query;
    INT         result;
    INT64       affectedRows;
    SIZE_T      groupNameLength;
    MYSQL_BIND  bind[1];
    MYSQL_STMT  *stmt;

    ASSERT(db != NULL);
    ASSERT(groupName != NULL);
    TRACE("db=%p groupName=%s\n", db, groupName);

    stmt = db->stmt[0];

    groupNameLength = strlen(groupName);

    //
    // Prepare statement and bind parameters
    //

    query = "DELETE FROM io_groups WHERE name=?";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    DB_CHECK_PARAMS(bind, stmt);
    ZeroMemory(&bind, sizeof(bind));

    bind[0].buffer_type   = MYSQL_TYPE_STRING;
    bind[0].buffer        = groupName;
    bind[0].buffer_length = groupNameLength;

    result = mysql_stmt_bind_param(stmt, bind);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmt));
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
    // Check for deleted rows
    //

    affectedRows = mysql_stmt_affected_rows(stmt);
    if (affectedRows == 0) {
        TRACE("Unable to delete group (no affected rows).\n");
        return ERROR_GROUP_NOT_FOUND;
    }

    return ERROR_SUCCESS;
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
    TRACE("db=%p groupName=%s groupFile=%p\n", db, groupName, groupFile);

    stmt = db->stmt[0];

    //
    // Prepare statement and bind parameters
    //

    query = "UPDATE io_groups SET lockowner=?, locktime=UNIX_TIMESTAMP()"
            "  WHERE name=?"
            "    AND lockowner IS NULL OR (UNIX_TIMESTAMP() - locktime) > ?";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    DB_CHECK_PARAMS(bind, stmt);
    ZeroMemory(&bind, sizeof(bind));

    bind[0].buffer_type   = MYSQL_TYPE_STRING;
    bind[0].buffer        = dbConfigLock.owner;
    bind[0].buffer_length = dbConfigLock.ownerLength;

    bind[1].buffer_type   = MYSQL_TYPE_STRING;
    bind[1].buffer        = groupName;
    bind[1].buffer_length = strlen(groupName);

    bind[2].buffer_type   = MYSQL_TYPE_LONG;
    bind[2].buffer        = &dbConfigLock.expire;

    result = mysql_stmt_bind_param(stmt, bind);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmt));
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
    // Check for modified rows
    //

    affectedRows = mysql_stmt_affected_rows(stmt);
    if (affectedRows == 0) {
        TRACE("Unable to lock group (no affected rows).\n");
        return ERROR_GROUP_LOCK_FAILED;
    }

    //
    // Update group data
    //

    error = DbGroupRead(db, groupName, groupFile);
    if (error != ERROR_SUCCESS) {
        TRACE("Unable to update group (error %lu).\n", error);
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
    TRACE("db=%p groupName=%s\n", db, groupName);

    stmt = db->stmt[0];

    //
    // Prepare statement and bind parameters
    //

    query = "UPDATE io_groups SET lockowner=NULL, locktime=0"
            "  WHERE name=? AND lockowner=?";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmt));
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
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmt));
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

    affectedRows = mysql_stmt_affected_rows(stmt);
    if (affectedRows == 0) {
        // Failure is acceptable
        TRACE("Unable to unlock group (no affected rows).\n");
    }

    return ERROR_SUCCESS;
}

DWORD DbGroupOpen(DB_CONTEXT *db, CHAR *groupName, GROUPFILE *groupFile)
{
    ASSERT(db != NULL);
    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);
    TRACE("db=%p groupName=%s groupFile=%p\n", db, groupName, groupFile);

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
    TRACE("db=%p groupName=%s groupFile=%p\n", db, groupName, groupFile);

    stmt = db->stmt[0];

    groupNameLength = strlen(groupName);

    //
    // Prepare statement and bind parameters
    //

    query = "UPDATE io_groups SET description=?, slots=?,"
            " users=?, vfsfile=?, updated=UNIX_TIMESTAMP()"
            "   WHERE name=?";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmt));
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
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmt));
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

    return ERROR_SUCCESS;
}

DWORD DbGroupClose(GROUPFILE *groupFile)
{
    UNREFERENCED_PARAMETER(groupFile);
    return ERROR_SUCCESS;
}

DWORD DbGroupRefresh(DB_CONTEXT *db)
{
    ASSERT(db != NULL);
    TRACE("db=%p\n", db);

    // TODO

    return ERROR_INTERNAL_ERROR;
}
