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

static DWORD DbGroupRead(DB_CONTEXT *db, CHAR *groupName, GROUPFILE *groupFile)
{
    ASSERT(db != NULL);
    ASSERT(groupName != NULL);
    ASSERT(groupFile != NULL);
    TRACE("db=%p groupName=%s groupFile=%p\n", db, groupName, groupFile);

    // TODO

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
    // Prepare and bind statement
    //

    query = "INSERT INTO io_groups"
            " (name,description,slots,users,vfsfile,updated)"
            " VALUES(?,?,?,?,?,UNIX_TIMESTAMP())";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmt));
        return DbMapError(result);
    }

    DB_CHECK_BINDS(bind, stmt);
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
        return DbMapError(result);
    }

    //
    // Execute prepared statement
    //

    result = mysql_stmt_execute(stmt);
    if (result != 0) {
        TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmt));
        return DbMapError(result);
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
    // Prepare and bind statement
    //

    query = "DELETE FROM io_groups WHERE name=?";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmt));
        return DbMapError(result);
    }

    DB_CHECK_BINDS(bind, stmt);
    ZeroMemory(&bind, sizeof(bind));

    bind[0].buffer_type   = MYSQL_TYPE_STRING;
    bind[0].buffer        = groupName;
    bind[0].buffer_length = groupNameLength;

    result = mysql_stmt_bind_param(stmt, bind);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmt));
        return DbMapError(result);
    }

    //
    // Execute prepared statement
    //

    result = mysql_stmt_execute(stmt);
    if (result != 0) {
        TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmt));
        return DbMapError(result);
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
    // Prepare and bind statement
    //

    query = "UPDATE io_groups SET lockowner=?, locktime=UNIX_TIMESTAMP()"
            "  WHERE name=?"
            "    AND lockowner IS NULL OR (UNIX_TIMESTAMP() - locktime) > ?";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmt));
        return DbMapError(result);
    }

    DB_CHECK_BINDS(bind, stmt);
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
        return DbMapError(result);
    }

    //
    // Execute prepared statement
    //

    result = mysql_stmt_execute(stmt);
    if (result != 0) {
        TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmt));
        return DbMapError(result);
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
    // Prepare and bind statement
    //

    query = "UPDATE io_groups SET lockowner=NULL, locktime=0"
            "  WHERE name=? AND lockowner=?";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmt));
        return DbMapError(result);
    }

    DB_CHECK_BINDS(bind, stmt);
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
        return DbMapError(result);
    }

    //
    // Execute prepared statement
    //

    result = mysql_stmt_execute(stmt);
    if (result != 0) {
        TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmt));
        return DbMapError(result);
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
    // Prepare and bind statement
    //

    query = "UPDATE io_groups SET description=?, slots=?,"
            " users=?, vfsfile=?, updated=UNIX_TIMESTAMP()"
            "   WHERE name=?";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmt));
        return DbMapError(result);
    }

    DB_CHECK_BINDS(bind, stmt);
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
        return DbMapError(result);
    }

    //
    // Execute prepared statement
    //

    result = mysql_stmt_execute(stmt);
    if (result != 0) {
        TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmt));
        return DbMapError(result);
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
