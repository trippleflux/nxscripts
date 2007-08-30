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
    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    ASSERT(newName != NULL);
    TRACE("db=%p userName=%s newName=%s\n", db, userName, newName);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserDelete(DB_CONTEXT *db, CHAR *userName)
{
    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    TRACE("db=%p userName=%s\n", db, userName);

    return ERROR_INTERNAL_ERROR;
}

DWORD DbUserLock(DB_CONTEXT *db, CHAR *userName)
{
    CHAR        *lockOwner;
    INT         lockExpire;
    INT         lockTimeout;
    CHAR        *query;
    INT         result;
    INT64       affectedRows;
    MYSQL_BIND  binds[3];

    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    TRACE("db=%p userName=%s\n", db, userName);

    DbGetConfig(&lockExpire, &lockTimeout, &lockOwner);

    //
    // Prepare statement
    //

    // TODO: lockTimeout

    query = "UPDATE io_users SET lockowner=?, locktime=UNIX_TIMESTAMP()"
            "  WHERE name=?"
            "    AND lockowner IS NULL OR (UNIX_TIMESTAMP() - locktime) > ?";

    result = mysql_stmt_prepare(db->stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(db->stmt));
        return DbMapError(result);
    }

    //
    // Bind parameters
    //

    DB_CHECK_BINDS(binds, db);
    ZeroMemory(&binds, sizeof(binds));

    binds[0].buffer_type   = MYSQL_TYPE_STRING;
    binds[0].buffer        = lockOwner;
    binds[0].buffer_length = strlen(lockOwner);

    binds[1].buffer_type   = MYSQL_TYPE_STRING;
    binds[1].buffer        = userName;
    binds[1].buffer_length = strlen(userName);

    binds[2].buffer_type   = MYSQL_TYPE_LONG;
    binds[2].buffer        = &lockExpire;

    result = mysql_stmt_bind_param(db->stmt, binds);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(db->stmt));
        return DbMapError(result);
    }

    //
    // Execute statement
    //

    result = mysql_stmt_execute(db->stmt);
    if (result != 0) {
        TRACE("Unable to execute statement: %s\n", mysql_stmt_error(db->stmt));
        return DbMapError(result);
    }

    affectedRows = mysql_stmt_affected_rows(db->stmt);
    if (affectedRows == 0) {
        TRACE("Unable to lock user, no affected rows.\n");
        return ERROR_LOCKED;
    }

    return ERROR_SUCCESS;
}

DWORD DbUserUnlock(DB_CONTEXT *db, CHAR *userName)
{
    CHAR        *lockOwner;
    CHAR        *query;
    INT         result;
    INT64       affectedRows;
    MYSQL_BIND  binds[2];

    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    TRACE("db=%p userName=%s\n", db, userName);

    DbGetConfig(NULL, NULL, &lockOwner);

    //
    // Prepare statement
    //

    query = "UPDATE io_users SET lockowner=NULL, locktime=0"
            "  WHERE name=? AND lockowner=?";

    result = mysql_stmt_prepare(db->stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(db->stmt));
        return DbMapError(result);
    }

    //
    // Bind parameters
    //

    DB_CHECK_BINDS(binds, db);
    ZeroMemory(&binds, sizeof(binds));

    binds[0].buffer_type   = MYSQL_TYPE_STRING;
    binds[0].buffer        = userName;
    binds[0].buffer_length = strlen(userName);

    binds[1].buffer_type   = MYSQL_TYPE_STRING;
    binds[1].buffer        = lockOwner;
    binds[1].buffer_length = strlen(lockOwner);

    result = mysql_stmt_bind_param(db->stmt, binds);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(db->stmt));
        return DbMapError(result);
    }

    //
    // Execute statement
    //

    result = mysql_stmt_execute(db->stmt);
    if (result != 0) {
        TRACE("Unable to execute statement: %s\n", mysql_stmt_error(db->stmt));
        return DbMapError(result);
    }

    affectedRows = mysql_stmt_affected_rows(db->stmt);
    if (affectedRows == 0) {
        // Failure is acceptable at this point.
        TRACE("Unable to unlock user, no affected rows.\n");
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

DWORD DbUserWrite(DB_CONTEXT *db, USERFILE *userFile)
{
    ASSERT(db != NULL);
    ASSERT(userFile != NULL);
    TRACE("db=%p userFile=%p\n", db, userFile);

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
