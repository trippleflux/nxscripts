/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    User Database Backend

Author:
    neoxed (neoxed@gmail.com) Jun 5, 2006

Abstract:
    User database storage backend.

*/

#include <base.h>
#include <backends.h>
#include <database.h>

DWORD DbUserCreate(DB_CONTEXT *db, CHAR *userName, USERFILE *userFile)
{
    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    TRACE("db=%p userName=%s userFile=%p\n", db, userName, userFile);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserRename(DB_CONTEXT *db, CHAR *userName, CHAR *newName)
{
    CHAR        *query;
    INT         result;
    INT64       affectedRows;
    MYSQL_BIND  bindUsers[2];
    MYSQL_BIND  bindAdmins[2];
    MYSQL_BIND  bindGroups[2];
    MYSQL_BIND  bindHosts[2];
    MYSQL_STMT  *stmtUsers;
    MYSQL_STMT  *stmtAdmins;
    MYSQL_STMT  *stmtGroups;
    MYSQL_STMT  *stmtHosts;

    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    ASSERT(newName != NULL);
    TRACE("db=%p userName=%s newName=%s\n", db, userName, newName);

    stmtUsers  = db->stmt[0];
    stmtAdmins = db->stmt[1];
    stmtGroups = db->stmt[2];
    stmtHosts  = db->stmt[3];

    //
    // Prepare and bind users statement
    //

    query = "UPDATE io_users SET name=?, updated=UNIX_TIMESTAMP() WHERE name=?";

    result = mysql_stmt_prepare(stmtUsers, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtUsers));
        return DbMapError(result);
    }

    DB_CHECK_BINDS(bindUsers, stmtUsers);
    ZeroMemory(&bindUsers, sizeof(bindUsers));

    bindUsers[0].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[0].buffer        = newName;
    bindUsers[0].buffer_length = strlen(newName);

    bindUsers[1].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[1].buffer        = userName;
    bindUsers[1].buffer_length = strlen(userName);

    result = mysql_stmt_bind_param(stmtUsers, bindUsers);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmtUsers));
        return DbMapError(result);
    }

    //
    // Prepare and bind admins statement
    //

    query = "UPDATE io_user_admins SET uname=? WHERE uname=?";

    result = mysql_stmt_prepare(stmtAdmins, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtAdmins));
        return DbMapError(result);
    }

    DB_CHECK_BINDS(bindAdmins, stmtAdmins);
    ZeroMemory(&bindAdmins, sizeof(bindAdmins));

    bindAdmins[0].buffer_type   = MYSQL_TYPE_STRING;
    bindAdmins[0].buffer        = newName;
    bindAdmins[0].buffer_length = strlen(newName);

    bindAdmins[1].buffer_type   = MYSQL_TYPE_STRING;
    bindAdmins[1].buffer        = userName;
    bindAdmins[1].buffer_length = strlen(userName);

    result = mysql_stmt_bind_param(stmtAdmins, bindAdmins);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmtAdmins));
        return DbMapError(result);
    }

    //
    // Prepare and bind groups statement
    //

    query = "UPDATE io_user_groups SET uname=? WHERE uname=?";

    result = mysql_stmt_prepare(stmtGroups, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtGroups));
        return DbMapError(result);
    }

    DB_CHECK_BINDS(bindGroups, stmtGroups);
    ZeroMemory(&bindGroups, sizeof(bindGroups));

    bindGroups[0].buffer_type   = MYSQL_TYPE_STRING;
    bindGroups[0].buffer        = newName;
    bindGroups[0].buffer_length = strlen(newName);

    bindGroups[1].buffer_type   = MYSQL_TYPE_STRING;
    bindGroups[1].buffer        = userName;
    bindGroups[1].buffer_length = strlen(userName);

    result = mysql_stmt_bind_param(stmtGroups, bindGroups);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmtGroups));
        return DbMapError(result);
    }

    //
    // Prepare and bind hosts statement
    //

    query = "UPDATE io_user_hosts SET name=? WHERE name=?";

    result = mysql_stmt_prepare(stmtHosts, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtHosts));
        return DbMapError(result);
    }

    DB_CHECK_BINDS(bindHosts, stmtHosts);
    ZeroMemory(&bindHosts, sizeof(bindHosts));

    bindHosts[0].buffer_type   = MYSQL_TYPE_STRING;
    bindHosts[0].buffer        = newName;
    bindHosts[0].buffer_length = strlen(newName);

    bindHosts[1].buffer_type   = MYSQL_TYPE_STRING;
    bindHosts[1].buffer        = userName;
    bindHosts[1].buffer_length = strlen(userName);

    result = mysql_stmt_bind_param(stmtHosts, bindHosts);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmtHosts));
        return DbMapError(result);
    }

    //
    // Begin transaction
    //

    result = mysql_query(db->handle, "START TRANSACTION");
    if (result != 0) {
        TRACE("Unable to start transaction: %s\n", mysql_error(db->handle));
        return DbMapError(result);
    }

    //
    // Execute prepared statements
    //

    result = mysql_stmt_execute(stmtUsers);
    if (result != 0) {
        TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmtUsers));
    }

    result = mysql_stmt_execute(stmtAdmins);
    if (result != 0) {
        TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmtAdmins));
    }

    result = mysql_stmt_execute(stmtGroups);
    if (result != 0) {
        TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmtGroups));
    }

    result = mysql_stmt_execute(stmtHosts);
    if (result != 0) {
        TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmtHosts));
    }

    //
    // Commit transaction
    //

    result = mysql_query(db->handle, "COMMIT");
    if (result != 0) {
        TRACE("Unable to commit transaction: %s\n", mysql_error(db->handle));
        return DbMapError(result);
    }

    //
    // Check for modified rows
    //

    affectedRows = mysql_stmt_affected_rows(stmtUsers);
    affectedRows += mysql_stmt_affected_rows(stmtAdmins);
    affectedRows += mysql_stmt_affected_rows(stmtGroups);
    affectedRows += mysql_stmt_affected_rows(stmtHosts);

    if (affectedRows == 0) {
        TRACE("Unable to rename user (no affected rows).\n");
        return ERROR_USER_NOT_FOUND;
    }

    return ERROR_SUCCESS;
}

DWORD DbUserDelete(DB_CONTEXT *db, CHAR *userName)
{
    CHAR        *query;
    INT         result;
    INT64       affectedRows;
    MYSQL_BIND  bindUsers[1];
    MYSQL_BIND  bindAdmins[1];
    MYSQL_BIND  bindGroups[1];
    MYSQL_BIND  bindHosts[1];
    MYSQL_STMT  *stmtUsers;
    MYSQL_STMT  *stmtAdmins;
    MYSQL_STMT  *stmtGroups;
    MYSQL_STMT  *stmtHosts;

    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    TRACE("db=%p userName=%s\n", db, userName);

    stmtUsers  = db->stmt[0];
    stmtAdmins = db->stmt[1];
    stmtGroups = db->stmt[2];
    stmtHosts  = db->stmt[3];

    //
    // Prepare and bind users statement
    //

    query = "DELETE FROM io_users WHERE name=?";

    result = mysql_stmt_prepare(stmtUsers, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtUsers));
        return DbMapError(result);
    }

    DB_CHECK_BINDS(bindUsers, stmtUsers);
    ZeroMemory(&bindUsers, sizeof(bindUsers));

    bindUsers[0].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[0].buffer        = userName;
    bindUsers[0].buffer_length = strlen(userName);

    result = mysql_stmt_bind_param(stmtUsers, bindUsers);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmtUsers));
        return DbMapError(result);
    }

    //
    // Prepare and bind admins statement
    //

    query = "DELETE FROM io_user_admins WHERE uname=?";

    result = mysql_stmt_prepare(stmtAdmins, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtAdmins));
        return DbMapError(result);
    }

    DB_CHECK_BINDS(bindAdmins, stmtAdmins);
    ZeroMemory(&bindAdmins, sizeof(bindAdmins));

    bindAdmins[0].buffer_type   = MYSQL_TYPE_STRING;
    bindAdmins[0].buffer        = userName;
    bindAdmins[0].buffer_length = strlen(userName);

    result = mysql_stmt_bind_param(stmtAdmins, bindAdmins);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmtAdmins));
        return DbMapError(result);
    }

    //
    // Prepare and bind groups statement
    //

    query = "DELETE FROM io_user_groups WHERE uname=?";

    result = mysql_stmt_prepare(stmtGroups, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtGroups));
        return DbMapError(result);
    }

    DB_CHECK_BINDS(bindGroups, stmtGroups);
    ZeroMemory(&bindGroups, sizeof(bindGroups));

    bindGroups[0].buffer_type   = MYSQL_TYPE_STRING;
    bindGroups[0].buffer        = userName;
    bindGroups[0].buffer_length = strlen(userName);

    result = mysql_stmt_bind_param(stmtGroups, bindGroups);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmtGroups));
        return DbMapError(result);
    }

    //
    // Prepare and bind hosts statement
    //

    query = "DELETE FROM io_user_hosts WHERE name=?";

    result = mysql_stmt_prepare(stmtHosts, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtHosts));
        return DbMapError(result);
    }

    DB_CHECK_BINDS(bindHosts, stmtHosts);
    ZeroMemory(&bindHosts, sizeof(bindHosts));

    bindHosts[0].buffer_type   = MYSQL_TYPE_STRING;
    bindHosts[0].buffer        = userName;
    bindHosts[0].buffer_length = strlen(userName);

    result = mysql_stmt_bind_param(stmtHosts, bindHosts);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmtHosts));
        return DbMapError(result);
    }

    //
    // Begin transaction
    //

    result = mysql_query(db->handle, "START TRANSACTION");
    if (result != 0) {
        TRACE("Unable to start transaction: %s\n", mysql_error(db->handle));
        return DbMapError(result);
    }

    //
    // Execute prepared statements
    //

    result = mysql_stmt_execute(stmtUsers);
    if (result != 0) {
        TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmtUsers));
    }

    result = mysql_stmt_execute(stmtAdmins);
    if (result != 0) {
        TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmtAdmins));
    }

    result = mysql_stmt_execute(stmtGroups);
    if (result != 0) {
        TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmtGroups));
    }

    result = mysql_stmt_execute(stmtHosts);
    if (result != 0) {
        TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmtHosts));
    }

    //
    // Commit transaction
    //

    result = mysql_query(db->handle, "COMMIT");
    if (result != 0) {
        TRACE("Unable to commit transaction: %s\n", mysql_error(db->handle));
        return DbMapError(result);
    }

    //
    // Check for deleted rows
    //

    affectedRows = mysql_stmt_affected_rows(stmtUsers);
    affectedRows += mysql_stmt_affected_rows(stmtAdmins);
    affectedRows += mysql_stmt_affected_rows(stmtGroups);
    affectedRows += mysql_stmt_affected_rows(stmtHosts);

    if (affectedRows == 0) {
        TRACE("Unable to delete user (no affected rows).\n");
        return ERROR_USER_NOT_FOUND;
    }

    return ERROR_SUCCESS;
}

DWORD DbUserLock(DB_CONTEXT *db, CHAR *userName, USERFILE *userFile)
{
    CHAR        *lockOwner;
    INT         lockExpire;
    INT         lockTimeout;
    CHAR        *query;
    INT         result;
    INT64       affectedRows;
    MYSQL_BIND  bind[3];
    MYSQL_STMT  *stmt;

    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    TRACE("db=%p userName=%s userFile=%p\n", db, userName, userFile);

    stmt = db->stmt[0];

    DbGetConfig(&lockExpire, &lockTimeout, &lockOwner);

    //
    // Prepare statement (TODO: timeout)
    //

    query = "UPDATE io_users SET lockowner=?, locktime=UNIX_TIMESTAMP()"
            "  WHERE name=?"
            "    AND lockowner IS NULL OR (UNIX_TIMESTAMP() - locktime) > ?";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmt));
        return DbMapError(result);
    }

    //
    // Bind parameters
    //

    DB_CHECK_BINDS(bind, stmt);
    ZeroMemory(&bind, sizeof(bind));

    bind[0].buffer_type   = MYSQL_TYPE_STRING;
    bind[0].buffer        = lockOwner;
    bind[0].buffer_length = strlen(lockOwner);

    bind[1].buffer_type   = MYSQL_TYPE_STRING;
    bind[1].buffer        = userName;
    bind[1].buffer_length = strlen(userName);

    bind[2].buffer_type   = MYSQL_TYPE_LONG;
    bind[2].buffer        = &lockExpire;

    result = mysql_stmt_bind_param(stmt, bind);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmt));
        return DbMapError(result);
    }

    //
    // Execute statement
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
        TRACE("Unable to lock user (no affected rows).\n");
        return ERROR_USER_LOCK_FAILED;
    }

    return ERROR_SUCCESS;
}

DWORD DbUserUnlock(DB_CONTEXT *db, CHAR *userName)
{
    CHAR        *lockOwner;
    CHAR        *query;
    INT         result;
    INT64       affectedRows;
    MYSQL_BIND  bind[2];
    MYSQL_STMT  *stmt;

    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    TRACE("db=%p userName=%s\n", db, userName);

    stmt = db->stmt[0];

    DbGetConfig(NULL, NULL, &lockOwner);

    //
    // Prepare statement
    //

    query = "UPDATE io_users SET lockowner=NULL, locktime=0"
            "  WHERE name=? AND lockowner=?";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmt));
        return DbMapError(result);
    }

    //
    // Bind parameters
    //

    DB_CHECK_BINDS(bind, stmt);
    ZeroMemory(&bind, sizeof(bind));

    bind[0].buffer_type   = MYSQL_TYPE_STRING;
    bind[0].buffer        = userName;
    bind[0].buffer_length = strlen(userName);

    bind[1].buffer_type   = MYSQL_TYPE_STRING;
    bind[1].buffer        = lockOwner;
    bind[1].buffer_length = strlen(lockOwner);

    result = mysql_stmt_bind_param(stmt, bind);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmt));
        return DbMapError(result);
    }

    //
    // Execute statement
    //

    result = mysql_stmt_execute(stmt);
    if (result != 0) {
        TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmt));
        return DbMapError(result);
    }

    affectedRows = mysql_stmt_affected_rows(stmt);
    if (affectedRows == 0) {
        // Failure is acceptable
        TRACE("Unable to unlock user (no affected rows).\n");
    }

    return ERROR_SUCCESS;
}

DWORD DbUserOpen(DB_CONTEXT *db, CHAR *userName, USERFILE *userFile)
{
    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    TRACE("db=%p userName=%s userFile=%p\n", db, userName, userFile);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserWrite(DB_CONTEXT *db, CHAR *userName, USERFILE *userFile)
{
    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    TRACE("db=%p userName=%s userFile=%p\n", db, userName, userFile);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserClose(USERFILE *userFile)
{
    ASSERT(userFile != NULL);
    TRACE("userFile=%p\n", userFile);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserRefresh(DB_CONTEXT *db)
{
    ASSERT(db != NULL);
    TRACE("db=%p\n", db);

    return ERROR_INTERNAL_ERROR;
}
