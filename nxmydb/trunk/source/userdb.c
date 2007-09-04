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

static BOOL GroupIdResolve(INT groupId, CHAR *buffer, SIZE_T bufferLength)
{
    CHAR *name;

    // Ignore the "NoGroup" group
    if (groupId == NOGROUP_ID) {
        return FALSE;
    }

    // Resolve the group ID to its name
    name = Io_Gid2Group(groupId);
    if (name == NULL) {
        return FALSE;
    }

    StringCchCopyA(buffer, bufferLength, name);
    return TRUE;
}

static DWORD DbUserRead(DB_CONTEXT *db, CHAR *userName, USERFILE *userFile)
{
    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    TRACE("db=%p userName=%s userFile=%p\n", db, userName, userFile);

    // TODO

    return ERROR_SUCCESS;
}

DWORD DbUserCreate(DB_CONTEXT *db, CHAR *userName, USERFILE *userFile)
{
    CHAR        buffer[128];
    CHAR        *host;
    CHAR        *query;
    INT         i;
    INT         id;
    INT         result;
    SIZE_T      userNameLength;
    MYSQL_BIND  bindAdmins[2];
    MYSQL_BIND  bindGroups[3];
    MYSQL_BIND  bindHosts[2];
    MYSQL_BIND  bindUsers[17];
    MYSQL_STMT  *stmtAdmins;
    MYSQL_STMT  *stmtGroups;
    MYSQL_STMT  *stmtHosts;
    MYSQL_STMT  *stmtUsers;

    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    TRACE("db=%p userName=%s userFile=%p\n", db, userName, userFile);

    ASSERT(sizeof(buffer) > _IP_LINE_LENGTH);
    ASSERT(sizeof(buffer) > _MAX_NAME);
    ZeroMemory(buffer, sizeof(buffer));

    stmtUsers  = db->stmt[0];
    stmtAdmins = db->stmt[1];
    stmtGroups = db->stmt[2];
    stmtHosts  = db->stmt[3];

    userNameLength = strlen(userName);

    //
    // Prepare users statement and bind parameters
    //

    query = "INSERT INTO io_users"
            "("
            "name,description,flags,home,limits,password,vfsfile,credits,ratio,"
            "alldn,allup,daydn,dayup,monthdn,monthup,wkdn,wkup,updated"
            ")"
            " VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,UNIX_TIMESTAMP())";

    result = mysql_stmt_prepare(stmtUsers, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtUsers));
        return DbMapError(result);
    }

    DB_CHECK_PARAMS(bindUsers, stmtUsers);
    ZeroMemory(&bindUsers, sizeof(bindUsers));

    bindUsers[0].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[0].buffer        = userName;
    bindUsers[0].buffer_length = userNameLength;

    bindUsers[1].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[1].buffer        = userFile->Tagline;
    bindUsers[1].buffer_length = sizeof(userFile->Tagline) - 1;

    bindUsers[2].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[2].buffer        = userFile->Flags;
    bindUsers[2].buffer_length = sizeof(userFile->Flags) - 1;

    bindUsers[3].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[3].buffer        = userFile->Home;
    bindUsers[3].buffer_length = sizeof(userFile->Home) - 1;

    bindUsers[4].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[4].buffer        = &userFile->Limits;
    bindUsers[4].buffer_length = sizeof(userFile->Limits);

    bindUsers[5].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[5].buffer        = &userFile->Password;
    bindUsers[5].buffer_length = sizeof(userFile->Password);

    bindUsers[6].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[6].buffer        = userFile->MountFile;
    bindUsers[6].buffer_length = sizeof(userFile->MountFile) - 1;

    bindUsers[7].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[7].buffer        = &userFile->Ratio;
    bindUsers[7].buffer_length = sizeof(userFile->Ratio);

    bindUsers[8].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[8].buffer        = &userFile->Credits;
    bindUsers[8].buffer_length = sizeof(userFile->Credits);

    bindUsers[9].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[9].buffer        = &userFile->DayUp;
    bindUsers[9].buffer_length = sizeof(userFile->DayUp);

    bindUsers[10].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[10].buffer        = &userFile->DayDn;
    bindUsers[10].buffer_length = sizeof(userFile->DayDn);

    bindUsers[11].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[11].buffer        = &userFile->WkUp;
    bindUsers[11].buffer_length = sizeof(userFile->WkUp);

    bindUsers[12].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[12].buffer        = &userFile->WkDn;
    bindUsers[12].buffer_length = sizeof(userFile->WkDn);

    bindUsers[13].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[13].buffer        = &userFile->MonthUp;
    bindUsers[13].buffer_length = sizeof(userFile->MonthUp);

    bindUsers[14].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[14].buffer        = &userFile->MonthDn;
    bindUsers[14].buffer_length = sizeof(userFile->MonthDn);

    bindUsers[15].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[15].buffer        = &userFile->AllUp;
    bindUsers[15].buffer_length = sizeof(userFile->AllUp);

    bindUsers[16].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[16].buffer        = &userFile->AllDn;
    bindUsers[16].buffer_length = sizeof(userFile->AllDn);

    result = mysql_stmt_bind_param(stmtUsers, bindUsers);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmtUsers));
        return DbMapError(result);
    }

    //
    // Prepare admins statement and bind parameters
    //

    query = "REPLACE INTO io_user_admins(uname,gname) VALUES(?,?)";

    result = mysql_stmt_prepare(stmtAdmins, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtAdmins));
        return DbMapError(result);
    }

    DB_CHECK_PARAMS(bindAdmins, stmtAdmins);
    ZeroMemory(&bindAdmins, sizeof(bindAdmins));

    bindAdmins[0].buffer_type   = MYSQL_TYPE_STRING;
    bindAdmins[0].buffer        = userName;
    bindAdmins[0].buffer_length = userNameLength;

    bindAdmins[1].buffer_type   = MYSQL_TYPE_STRING;
    bindAdmins[1].buffer        = &buffer;
    bindAdmins[1].buffer_length = _MAX_NAME;

    result = mysql_stmt_bind_param(stmtAdmins, bindAdmins);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmtAdmins));
        return DbMapError(result);
    }

    //
    // Prepare groups statement and bind parameters
    //

    query = "REPLACE INTO io_user_groups(uname,gname,idx) VALUES(?,?,?)";

    result = mysql_stmt_prepare(stmtGroups, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtGroups));
        return DbMapError(result);
    }

    DB_CHECK_PARAMS(bindGroups, stmtGroups);
    ZeroMemory(&bindGroups, sizeof(bindGroups));

    bindGroups[0].buffer_type   = MYSQL_TYPE_STRING;
    bindGroups[0].buffer        = userName;
    bindGroups[0].buffer_length = userNameLength;

    bindGroups[1].buffer_type   = MYSQL_TYPE_STRING;
    bindGroups[1].buffer        = &buffer;
    bindGroups[1].buffer_length = _MAX_NAME;

    bindGroups[2].buffer_type   = MYSQL_TYPE_LONG;
    bindGroups[2].buffer        = &i;

    result = mysql_stmt_bind_param(stmtGroups, bindGroups);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmtGroups));
        return DbMapError(result);
    }

    //
    // Prepare hosts statement and bind parameters
    //

    query = "REPLACE INTO io_user_hosts(name,host) VALUES(?,?)";

    result = mysql_stmt_prepare(stmtHosts, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtHosts));
        return DbMapError(result);
    }

    DB_CHECK_PARAMS(bindHosts, stmtHosts);
    ZeroMemory(&bindHosts, sizeof(bindHosts));

    bindHosts[0].buffer_type   = MYSQL_TYPE_STRING;
    bindHosts[0].buffer        = userName;
    bindHosts[0].buffer_length = userNameLength;

    bindHosts[1].buffer_type   = MYSQL_TYPE_STRING;
    bindHosts[1].buffer        = &buffer;
    bindHosts[1].buffer_length = _IP_LINE_LENGTH;

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
        goto rollback;
    }

    for (i = 0; i < MAX_GROUPS; i++) {
        id = userFile->AdminGroups[i];
        if (id == INVALID_GROUP) {
            break;
        }
        if (!GroupIdResolve(id, buffer, ELEMENT_COUNT(buffer))) {
            continue;
        }

        result = mysql_stmt_execute(stmtAdmins);
        if (result != 0) {
            TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmtAdmins));
            goto rollback;
        }
    }

    for (i = 0; i < MAX_GROUPS; i++) {
        id = userFile->Groups[i];
        if (id == INVALID_GROUP) {
            break;
        }
        if (!GroupIdResolve(id, buffer, ELEMENT_COUNT(buffer))) {
            continue;
        }

        result = mysql_stmt_execute(stmtGroups);
        if (result != 0) {
            TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmtGroups));
            goto rollback;
        }
    }

    for (i = 0; i < MAX_IPS; i++) {
        host = userFile->Ip[i];
        if (host[0] == '\0') {
            // The last IP is marked by a null
            break;
        }
        StringCchCopyA(buffer, ELEMENT_COUNT(buffer), host);

        result = mysql_stmt_execute(stmtHosts);
        if (result != 0) {
            TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmtHosts));
            goto rollback;
        }
    }

    //
    // Commit transaction
    //

    result = mysql_query(db->handle, "COMMIT");
    if (result != 0) {
        TRACE("Unable to commit transaction: %s\n", mysql_error(db->handle));
        return DbMapError(result);
    }

    return ERROR_SUCCESS;

rollback:
    //
    // Rollback transaction on error
    //

    if (mysql_query(db->handle, "ROLLBACK") != 0) {
        TRACE("Unable to commit transaction: %s\n", mysql_error(db->handle));
    }

    return DbMapError(result);
}

DWORD DbUserRename(DB_CONTEXT *db, CHAR *userName, CHAR *newName)
{
    CHAR        *query;
    INT         result;
    INT64       affectedRows;
    SIZE_T      userNameLength;
    SIZE_T      newNameLength;
    MYSQL_BIND  bindAdmins[2];
    MYSQL_BIND  bindGroups[2];
    MYSQL_BIND  bindHosts[2];
    MYSQL_BIND  bindUsers[2];
    MYSQL_STMT  *stmtAdmins;
    MYSQL_STMT  *stmtGroups;
    MYSQL_STMT  *stmtHosts;
    MYSQL_STMT  *stmtUsers;

    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    ASSERT(newName != NULL);
    TRACE("db=%p userName=%s newName=%s\n", db, userName, newName);

    stmtUsers  = db->stmt[0];
    stmtAdmins = db->stmt[1];
    stmtGroups = db->stmt[2];
    stmtHosts  = db->stmt[3];

    userNameLength = strlen(userName);
    newNameLength  = strlen(newName);

    //
    // Prepare users statement and bind parameters
    //

    query = "UPDATE io_users SET name=?, updated=UNIX_TIMESTAMP() WHERE name=?";

    result = mysql_stmt_prepare(stmtUsers, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtUsers));
        return DbMapError(result);
    }

    DB_CHECK_PARAMS(bindUsers, stmtUsers);
    ZeroMemory(&bindUsers, sizeof(bindUsers));

    bindUsers[0].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[0].buffer        = newName;
    bindUsers[0].buffer_length = newNameLength;

    bindUsers[1].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[1].buffer        = userName;
    bindUsers[1].buffer_length = userNameLength;

    result = mysql_stmt_bind_param(stmtUsers, bindUsers);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmtUsers));
        return DbMapError(result);
    }

    //
    // Prepare admins statement and bind parameters
    //

    query = "UPDATE io_user_admins SET uname=? WHERE uname=?";

    result = mysql_stmt_prepare(stmtAdmins, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtAdmins));
        return DbMapError(result);
    }

    DB_CHECK_PARAMS(bindAdmins, stmtAdmins);
    ZeroMemory(&bindAdmins, sizeof(bindAdmins));

    bindAdmins[0].buffer_type   = MYSQL_TYPE_STRING;
    bindAdmins[0].buffer        = newName;
    bindAdmins[0].buffer_length = newNameLength;

    bindAdmins[1].buffer_type   = MYSQL_TYPE_STRING;
    bindAdmins[1].buffer        = userName;
    bindAdmins[1].buffer_length = userNameLength;

    result = mysql_stmt_bind_param(stmtAdmins, bindAdmins);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmtAdmins));
        return DbMapError(result);
    }

    //
    // Prepare groups statement and bind parameters
    //

    query = "UPDATE io_user_groups SET uname=? WHERE uname=?";

    result = mysql_stmt_prepare(stmtGroups, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtGroups));
        return DbMapError(result);
    }

    DB_CHECK_PARAMS(bindGroups, stmtGroups);
    ZeroMemory(&bindGroups, sizeof(bindGroups));

    bindGroups[0].buffer_type   = MYSQL_TYPE_STRING;
    bindGroups[0].buffer        = newName;
    bindGroups[0].buffer_length = newNameLength;

    bindGroups[1].buffer_type   = MYSQL_TYPE_STRING;
    bindGroups[1].buffer        = userName;
    bindGroups[1].buffer_length = userNameLength;

    result = mysql_stmt_bind_param(stmtGroups, bindGroups);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmtGroups));
        return DbMapError(result);
    }

    //
    // Prepare hosts statement and bind parameters
    //

    query = "UPDATE io_user_hosts SET name=? WHERE name=?";

    result = mysql_stmt_prepare(stmtHosts, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtHosts));
        return DbMapError(result);
    }

    DB_CHECK_PARAMS(bindHosts, stmtHosts);
    ZeroMemory(&bindHosts, sizeof(bindHosts));

    bindHosts[0].buffer_type   = MYSQL_TYPE_STRING;
    bindHosts[0].buffer        = newName;
    bindHosts[0].buffer_length = newNameLength;

    bindHosts[1].buffer_type   = MYSQL_TYPE_STRING;
    bindHosts[1].buffer        = userName;
    bindHosts[1].buffer_length = userNameLength;

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
    SIZE_T      userNameLength;
    MYSQL_BIND  bindAdmins[1];
    MYSQL_BIND  bindGroups[1];
    MYSQL_BIND  bindHosts[1];
    MYSQL_BIND  bindUsers[1];
    MYSQL_STMT  *stmtAdmins;
    MYSQL_STMT  *stmtGroups;
    MYSQL_STMT  *stmtHosts;
    MYSQL_STMT  *stmtUsers;

    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    TRACE("db=%p userName=%s\n", db, userName);

    stmtUsers  = db->stmt[0];
    stmtAdmins = db->stmt[1];
    stmtGroups = db->stmt[2];
    stmtHosts  = db->stmt[3];

    userNameLength = strlen(userName);

    //
    // Prepare users statement and bind parameters
    //

    query = "DELETE FROM io_users WHERE name=?";

    result = mysql_stmt_prepare(stmtUsers, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtUsers));
        return DbMapError(result);
    }

    DB_CHECK_PARAMS(bindUsers, stmtUsers);
    ZeroMemory(&bindUsers, sizeof(bindUsers));

    bindUsers[0].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[0].buffer        = userName;
    bindUsers[0].buffer_length = userNameLength;

    result = mysql_stmt_bind_param(stmtUsers, bindUsers);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmtUsers));
        return DbMapError(result);
    }

    //
    // Prepare admins statement and bind parameters
    //

    query = "DELETE FROM io_user_admins WHERE uname=?";

    result = mysql_stmt_prepare(stmtAdmins, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtAdmins));
        return DbMapError(result);
    }

    DB_CHECK_PARAMS(bindAdmins, stmtAdmins);
    ZeroMemory(&bindAdmins, sizeof(bindAdmins));

    bindAdmins[0].buffer_type   = MYSQL_TYPE_STRING;
    bindAdmins[0].buffer        = userName;
    bindAdmins[0].buffer_length = userNameLength;

    result = mysql_stmt_bind_param(stmtAdmins, bindAdmins);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmtAdmins));
        return DbMapError(result);
    }

    //
    // Prepare groups statement and bind parameters
    //

    query = "DELETE FROM io_user_groups WHERE uname=?";

    result = mysql_stmt_prepare(stmtGroups, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtGroups));
        return DbMapError(result);
    }

    DB_CHECK_PARAMS(bindGroups, stmtGroups);
    ZeroMemory(&bindGroups, sizeof(bindGroups));

    bindGroups[0].buffer_type   = MYSQL_TYPE_STRING;
    bindGroups[0].buffer        = userName;
    bindGroups[0].buffer_length = userNameLength;

    result = mysql_stmt_bind_param(stmtGroups, bindGroups);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmtGroups));
        return DbMapError(result);
    }

    //
    // Prepare hosts statement and bind parameters
    //

    query = "DELETE FROM io_user_hosts WHERE name=?";

    result = mysql_stmt_prepare(stmtHosts, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtHosts));
        return DbMapError(result);
    }

    DB_CHECK_PARAMS(bindHosts, stmtHosts);
    ZeroMemory(&bindHosts, sizeof(bindHosts));

    bindHosts[0].buffer_type   = MYSQL_TYPE_STRING;
    bindHosts[0].buffer        = userName;
    bindHosts[0].buffer_length = userNameLength;

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
    CHAR        *query;
    DWORD       error;
    INT         result;
    INT64       affectedRows;
    MYSQL_BIND  bind[3];
    MYSQL_STMT  *stmt;

    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    TRACE("db=%p userName=%s userFile=%p\n", db, userName, userFile);

    stmt = db->stmt[0];

    //
    // Prepare statement and bind parameters
    //

    query = "UPDATE io_users SET lockowner=?, locktime=UNIX_TIMESTAMP()"
            "  WHERE name=?"
            "    AND lockowner IS NULL OR (UNIX_TIMESTAMP() - locktime) > ?";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmt));
        return DbMapError(result);
    }

    DB_CHECK_PARAMS(bind, stmt);
    ZeroMemory(&bind, sizeof(bind));

    bind[0].buffer_type   = MYSQL_TYPE_STRING;
    bind[0].buffer        = dbConfigLock.owner;
    bind[0].buffer_length = dbConfigLock.ownerLength;

    bind[1].buffer_type   = MYSQL_TYPE_STRING;
    bind[1].buffer        = userName;
    bind[1].buffer_length = strlen(userName);

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
        TRACE("Unable to lock user (no affected rows).\n");
        return ERROR_USER_LOCK_FAILED;
    }

    //
    // Update user data
    //

    error = DbUserRead(db, userName, userFile);
    if (error != ERROR_SUCCESS) {
        TRACE("Unable to update user (error %lu).\n", error);
    }

    return error;
}

DWORD DbUserUnlock(DB_CONTEXT *db, CHAR *userName)
{
    CHAR        *query;
    INT         result;
    INT64       affectedRows;
    MYSQL_BIND  bind[2];
    MYSQL_STMT  *stmt;

    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    TRACE("db=%p userName=%s\n", db, userName);

    stmt = db->stmt[0];

    //
    // Prepare statement and bind parameters
    //

    query = "UPDATE io_users SET lockowner=NULL, locktime=0"
            "  WHERE name=? AND lockowner=?";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmt));
        return DbMapError(result);
    }

    DB_CHECK_PARAMS(bind, stmt);
    ZeroMemory(&bind, sizeof(bind));

    bind[0].buffer_type   = MYSQL_TYPE_STRING;
    bind[0].buffer        = userName;
    bind[0].buffer_length = strlen(userName);

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

    return DbUserRead(db, userName, userFile);
}

DWORD DbUserWrite(DB_CONTEXT *db, CHAR *userName, USERFILE *userFile)
{
    CHAR        buffer[128];
    CHAR        *host;
    CHAR        *query;
    INT         i;
    INT         id;
    INT         result;
    SIZE_T      userNameLength;
    MYSQL_BIND  bindAddAdmins[2];
    MYSQL_BIND  bindAddGroups[3];
    MYSQL_BIND  bindAddHosts[2];
    MYSQL_BIND  bindDelAdmins[1];
    MYSQL_BIND  bindDelGroups[1];
    MYSQL_BIND  bindDelHosts[1];
    MYSQL_BIND  bindUsers[17];
    MYSQL_STMT  *stmtAddAdmins;
    MYSQL_STMT  *stmtAddGroups;
    MYSQL_STMT  *stmtAddHosts;
    MYSQL_STMT  *stmtDelAdmins;
    MYSQL_STMT  *stmtDelGroups;
    MYSQL_STMT  *stmtDelHosts;
    MYSQL_STMT  *stmtUsers;

    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    TRACE("db=%p userName=%s userFile=%p\n", db, userName, userFile);

    ASSERT(sizeof(buffer) > _IP_LINE_LENGTH);
    ASSERT(sizeof(buffer) > _MAX_NAME);
    ZeroMemory(buffer, sizeof(buffer));

    stmtUsers     = db->stmt[0];
    stmtAddAdmins = db->stmt[1];
    stmtAddGroups = db->stmt[2];
    stmtAddHosts  = db->stmt[3];
    stmtDelAdmins = db->stmt[4];
    stmtDelGroups = db->stmt[5];
    stmtDelHosts  = db->stmt[6];

    userNameLength = strlen(userName);

    //
    // Prepare users statement and bind parameters
    //

    query = "UPDATE io_users SET description=?, flags=?, home=?, limits=?,"
            " password=?, vfsfile=?, credits=?, ratio=?, alldn=?, allup=?,"
            " daydn=?, dayup=?, monthdn=?, monthup=?, wkdn=?, wkup=?,"
            " updated=UNIX_TIMESTAMP()"
            "   WHERE name=?";

    result = mysql_stmt_prepare(stmtUsers, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtUsers));
        return DbMapError(result);
    }

    DB_CHECK_PARAMS(bindUsers, stmtUsers);
    ZeroMemory(&bindUsers, sizeof(bindUsers));

    bindUsers[0].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[0].buffer        = userFile->Tagline;
    bindUsers[0].buffer_length = sizeof(userFile->Tagline) - 1;

    bindUsers[1].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[1].buffer        = userFile->Flags;
    bindUsers[1].buffer_length = sizeof(userFile->Flags) - 1;

    bindUsers[2].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[2].buffer        = userFile->Home;
    bindUsers[2].buffer_length = sizeof(userFile->Home) - 1;

    bindUsers[3].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[3].buffer        = &userFile->Limits;
    bindUsers[3].buffer_length = sizeof(userFile->Limits);

    bindUsers[4].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[4].buffer        = &userFile->Password;
    bindUsers[4].buffer_length = sizeof(userFile->Password);

    bindUsers[5].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[5].buffer        = userFile->MountFile;
    bindUsers[5].buffer_length = sizeof(userFile->MountFile) - 1;

    bindUsers[6].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[6].buffer        = &userFile->Ratio;
    bindUsers[6].buffer_length = sizeof(userFile->Ratio);

    bindUsers[7].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[7].buffer        = &userFile->Credits;
    bindUsers[7].buffer_length = sizeof(userFile->Credits);

    bindUsers[8].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[8].buffer        = &userFile->DayUp;
    bindUsers[8].buffer_length = sizeof(userFile->DayUp);

    bindUsers[9].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[9].buffer        = &userFile->DayDn;
    bindUsers[9].buffer_length = sizeof(userFile->DayDn);

    bindUsers[10].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[10].buffer        = &userFile->WkUp;
    bindUsers[10].buffer_length = sizeof(userFile->WkUp);

    bindUsers[11].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[11].buffer        = &userFile->WkDn;
    bindUsers[11].buffer_length = sizeof(userFile->WkDn);

    bindUsers[12].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[12].buffer        = &userFile->MonthUp;
    bindUsers[12].buffer_length = sizeof(userFile->MonthUp);

    bindUsers[13].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[13].buffer        = &userFile->MonthDn;
    bindUsers[13].buffer_length = sizeof(userFile->MonthDn);

    bindUsers[14].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[14].buffer        = &userFile->AllUp;
    bindUsers[14].buffer_length = sizeof(userFile->AllUp);

    bindUsers[15].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[15].buffer        = &userFile->AllDn;
    bindUsers[15].buffer_length = sizeof(userFile->AllDn);

    bindUsers[16].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[16].buffer        = userName;
    bindUsers[16].buffer_length = userNameLength;

    result = mysql_stmt_bind_param(stmtUsers, bindUsers);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmtUsers));
        return DbMapError(result);
    }

    //
    // Prepare admins statement and bind parameters
    //

    query = "REPLACE INTO io_user_admins(uname,gname) VALUES(?,?)";

    result = mysql_stmt_prepare(stmtAddAdmins, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtAddAdmins));
        return DbMapError(result);
    }

    DB_CHECK_PARAMS(bindAddAdmins, stmtAddAdmins);
    ZeroMemory(&bindAddAdmins, sizeof(bindAddAdmins));

    bindAddAdmins[0].buffer_type   = MYSQL_TYPE_STRING;
    bindAddAdmins[0].buffer        = userName;
    bindAddAdmins[0].buffer_length = userNameLength;

    bindAddAdmins[1].buffer_type   = MYSQL_TYPE_STRING;
    bindAddAdmins[1].buffer        = &buffer;
    bindAddAdmins[1].buffer_length = _MAX_NAME;

    result = mysql_stmt_bind_param(stmtAddAdmins, bindAddAdmins);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmtAddAdmins));
        return DbMapError(result);
    }

    //
    // Prepare groups statement and bind parameters
    //

    query = "REPLACE INTO io_user_groups(uname,gname,idx) VALUES(?,?,?)";

    result = mysql_stmt_prepare(stmtAddGroups, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtAddGroups));
        return DbMapError(result);
    }

    DB_CHECK_PARAMS(bindAddGroups, stmtAddGroups);
    ZeroMemory(&bindAddGroups, sizeof(bindAddGroups));

    bindAddGroups[0].buffer_type   = MYSQL_TYPE_STRING;
    bindAddGroups[0].buffer        = userName;
    bindAddGroups[0].buffer_length = userNameLength;

    bindAddGroups[1].buffer_type   = MYSQL_TYPE_STRING;
    bindAddGroups[1].buffer        = &buffer;
    bindAddGroups[1].buffer_length = _MAX_NAME;

    bindAddGroups[2].buffer_type   = MYSQL_TYPE_LONG;
    bindAddGroups[2].buffer        = &i;

    result = mysql_stmt_bind_param(stmtAddGroups, bindAddGroups);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmtAddGroups));
        return DbMapError(result);
    }

    //
    // Prepare hosts statement and bind parameters
    //

    query = "REPLACE INTO io_user_hosts(name,host) VALUES(?,?)";

    result = mysql_stmt_prepare(stmtAddHosts, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtAddHosts));
        return DbMapError(result);
    }

    DB_CHECK_PARAMS(bindAddHosts, stmtAddHosts);
    ZeroMemory(&bindAddHosts, sizeof(bindAddHosts));

    bindAddHosts[0].buffer_type   = MYSQL_TYPE_STRING;
    bindAddHosts[0].buffer        = userName;
    bindAddHosts[0].buffer_length = userNameLength;

    bindAddHosts[1].buffer_type   = MYSQL_TYPE_STRING;
    bindAddHosts[1].buffer        = &buffer;
    bindAddHosts[1].buffer_length = _IP_LINE_LENGTH;

    result = mysql_stmt_bind_param(stmtAddHosts, bindAddHosts);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmtAddHosts));
        return DbMapError(result);
    }

    //
    // Prepare and bind admins delete statement
    //

    query = "DELETE FROM io_user_admins WHERE uname=?";

    result = mysql_stmt_prepare(stmtDelAdmins, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtDelAdmins));
        return DbMapError(result);
    }

    DB_CHECK_PARAMS(bindDelAdmins, stmtDelAdmins);
    ZeroMemory(&bindDelAdmins, sizeof(bindDelAdmins));

    bindDelAdmins[0].buffer_type   = MYSQL_TYPE_STRING;
    bindDelAdmins[0].buffer        = userName;
    bindDelAdmins[0].buffer_length = userNameLength;

    result = mysql_stmt_bind_param(stmtDelAdmins, bindDelAdmins);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmtDelAdmins));
        return DbMapError(result);
    }

    //
    // Prepare and bind groups delete statement
    //

    query = "DELETE FROM io_user_groups WHERE uname=?";

    result = mysql_stmt_prepare(stmtDelGroups, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtDelGroups));
        return DbMapError(result);
    }

    DB_CHECK_PARAMS(bindDelGroups, stmtDelGroups);
    ZeroMemory(&bindDelGroups, sizeof(bindDelGroups));

    bindDelGroups[0].buffer_type   = MYSQL_TYPE_STRING;
    bindDelGroups[0].buffer        = userName;
    bindDelGroups[0].buffer_length = userNameLength;

    result = mysql_stmt_bind_param(stmtDelGroups, bindDelGroups);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmtDelGroups));
        return DbMapError(result);
    }

    //
    // Prepare and bind hosts delete statement
    //

    query = "DELETE FROM io_user_hosts WHERE name=?";

    result = mysql_stmt_prepare(stmtDelHosts, query, strlen(query));
    if (result != 0) {
        TRACE("Unable to prepare statement: %s\n", mysql_stmt_error(stmtDelHosts));
        return DbMapError(result);
    }

    DB_CHECK_PARAMS(bindDelHosts, stmtDelHosts);
    ZeroMemory(&bindDelHosts, sizeof(bindDelHosts));

    bindDelHosts[0].buffer_type   = MYSQL_TYPE_STRING;
    bindDelHosts[0].buffer        = userName;
    bindDelHosts[0].buffer_length = userNameLength;

    result = mysql_stmt_bind_param(stmtDelHosts, bindDelHosts);
    if (result != 0) {
        TRACE("Unable to bind parameters: %s\n", mysql_stmt_error(stmtDelHosts));
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
        goto rollback;
    }

    result = mysql_stmt_execute(stmtDelAdmins);
    if (result != 0) {
        TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmtDelAdmins));
        goto rollback;
    }

    for (i = 0; i < MAX_GROUPS; i++) {
        id = userFile->AdminGroups[i];
        if (id == INVALID_GROUP) {
            break;
        }
        if (!GroupIdResolve(id, buffer, ELEMENT_COUNT(buffer))) {
            continue;
        }

        result = mysql_stmt_execute(stmtAddAdmins);
        if (result != 0) {
            TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmtAddAdmins));
            goto rollback;
        }
    }

    result = mysql_stmt_execute(stmtDelGroups);
    if (result != 0) {
        TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmtDelGroups));
        goto rollback;
    }

    for (i = 0; i < MAX_GROUPS; i++) {
        id = userFile->Groups[i];
        if (id == INVALID_GROUP) {
            break;
        }
        if (!GroupIdResolve(id, buffer, ELEMENT_COUNT(buffer))) {
            continue;
        }

        result = mysql_stmt_execute(stmtAddGroups);
        if (result != 0) {
            TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmtAddGroups));
            goto rollback;
        }
    }

    result = mysql_stmt_execute(stmtDelHosts);
    if (result != 0) {
        TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmtDelHosts));
        goto rollback;
    }

    for (i = 0; i < MAX_IPS; i++) {
        host = userFile->Ip[i];
        if (host[0] == '\0') {
            // The last IP is marked by a null
            break;
        }
        StringCchCopyA(buffer, ELEMENT_COUNT(buffer), host);

        result = mysql_stmt_execute(stmtAddHosts);
        if (result != 0) {
            TRACE("Unable to execute statement: %s\n", mysql_stmt_error(stmtAddHosts));
            goto rollback;
        }
    }

    //
    // Commit transaction
    //

    result = mysql_query(db->handle, "COMMIT");
    if (result != 0) {
        TRACE("Unable to commit transaction: %s\n", mysql_error(db->handle));
        return DbMapError(result);
    }

    return ERROR_SUCCESS;

rollback:
    //
    // Rollback transaction on error
    //

    if (mysql_query(db->handle, "ROLLBACK") != 0) {
        TRACE("Unable to commit transaction: %s\n", mysql_error(db->handle));
    }

    return DbMapError(result);
}

DWORD DbUserClose(USERFILE *userFile)
{
    UNREFERENCED_PARAMETER(userFile);
    return ERROR_SUCCESS;
}

DWORD DbUserRefresh(DB_CONTEXT *db)
{
    ASSERT(db != NULL);
    TRACE("db=%p\n", db);

    // TODO

    return ERROR_INTERNAL_ERROR;
}
