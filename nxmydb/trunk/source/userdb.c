/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2009 neoxed

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

static VOID CopyHost(CHAR *host, CHAR *buffer, SIZE_T inLength, SIZE_T *outLength)
{
    size_t remaining;

    ASSERT(host != NULL);
    ASSERT(buffer != NULL);
    ASSERT(outLength != NULL);

    StringCchCopyExA(buffer, inLength, host, NULL, &remaining, 0);
    *outLength = inLength - remaining;
}

static BOOL CopyGroup(INT groupId, CHAR *buffer, SIZE_T inLength, SIZE_T *outLength)
{
    CHAR    *name;
    size_t  remaining;

    ASSERT(buffer != NULL);
    ASSERT(outLength != NULL);

    // Ignore the "NoGroup" group
    if (groupId == NOGROUP_ID) {
        return FALSE;
    }

    // Resolve the group ID to its name
    name = Io_Gid2Group(groupId);
    if (name == NULL) {
        return FALSE;
    }

    StringCchCopyExA(buffer, inLength, name, NULL, &remaining, 0);
    *outLength = inLength - remaining;
    return TRUE;
}

DWORD DbUserReadExtra(DB_CONTEXT *db, CHAR *userName, USERFILE *userFile)
{
    CHAR        buffer[128];
    CHAR        *query;
    INT         index;
    INT         result;
    SIZE_T      userNameLength;
    MYSQL_BIND  bindInputAdmins[1];
    MYSQL_BIND  bindInputGroups[1];
    MYSQL_BIND  bindInputHosts[1];
    MYSQL_BIND  bindOutputAdmins[1];
    MYSQL_BIND  bindOutputGroups[1];
    MYSQL_BIND  bindOutputHosts[1];
    MYSQL_STMT  *stmtAdmins;
    MYSQL_STMT  *stmtGroups;
    MYSQL_STMT  *stmtHosts;
    MYSQL_RES   *metadataAdmins;
    MYSQL_RES   *metadataGroups;
    MYSQL_RES   *metadataHosts;

    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    TRACE("db=%p userName=%s userFile=%p", db, userName, userFile);

    // Buffer must be large enough to hold a group-name or host-mask.
    ASSERT(sizeof(buffer) > _IP_LINE_LENGTH);
    ASSERT(sizeof(buffer) > _MAX_NAME);
    ZeroMemory(buffer, sizeof(buffer));

    stmtAdmins = db->stmt[0];
    stmtGroups = db->stmt[1];
    stmtHosts  = db->stmt[2];

    userNameLength = strlen(userName);

    //
    // Prepare admins statement and bind parameters
    //

    query = "SELECT gname FROM io_user_admins WHERE uname=?";

    result = mysql_stmt_prepare(stmtAdmins, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtAdmins));
        return DbMapErrorFromStmt(stmtAdmins);
    }

    DB_CHECK_PARAMS(bindInputAdmins, stmtAdmins);
    ZeroMemory(&bindInputAdmins, sizeof(bindInputAdmins));

    // uname=?
    bindInputAdmins[0].buffer_type   = MYSQL_TYPE_STRING;
    bindInputAdmins[0].buffer        = userName;
    bindInputAdmins[0].buffer_length = userNameLength;

    result = mysql_stmt_bind_param(stmtAdmins, bindInputAdmins);
    if (result != 0) {
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtAdmins));
        return DbMapErrorFromStmt(stmtAdmins);
    }

    metadataAdmins = mysql_stmt_result_metadata(stmtAdmins);
    if (metadataAdmins == NULL) {
        LOG_WARN("Unable to retrieve result metadata: %s", mysql_stmt_error(stmtAdmins));
        return DbMapErrorFromStmt(stmtAdmins);
    }

    //
    // Prepare groups statement and bind parameters
    //

    query = "SELECT gname FROM io_user_groups WHERE uname=? ORDER BY idx ASC";

    result = mysql_stmt_prepare(stmtGroups, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtGroups));
        return DbMapErrorFromStmt(stmtGroups);
    }

    DB_CHECK_PARAMS(bindInputGroups, stmtGroups);
    ZeroMemory(&bindInputGroups, sizeof(bindInputGroups));

    // uname=?
    bindInputGroups[0].buffer_type   = MYSQL_TYPE_STRING;
    bindInputGroups[0].buffer        = userName;
    bindInputGroups[0].buffer_length = userNameLength;

    result = mysql_stmt_bind_param(stmtGroups, bindInputGroups);
    if (result != 0) {
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtGroups));
        return DbMapErrorFromStmt(stmtGroups);
    }

    metadataGroups = mysql_stmt_result_metadata(stmtGroups);
    if (metadataGroups == NULL) {
        LOG_WARN("Unable to retrieve result metadata: %s", mysql_stmt_error(stmtGroups));
        return DbMapErrorFromStmt(stmtGroups);
    }

    //
    // Prepare hosts statement and bind parameters
    //

    query = "SELECT host FROM io_user_hosts  WHERE uname=?";

    result = mysql_stmt_prepare(stmtHosts, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtHosts));
        return DbMapErrorFromStmt(stmtHosts);
    }

    DB_CHECK_PARAMS(bindInputHosts, stmtHosts);
    ZeroMemory(&bindInputHosts, sizeof(bindInputHosts));

    // uname=?
    bindInputHosts[0].buffer_type   = MYSQL_TYPE_STRING;
    bindInputHosts[0].buffer        = userName;
    bindInputHosts[0].buffer_length = userNameLength;

    result = mysql_stmt_bind_param(stmtHosts, bindInputHosts);
    if (result != 0) {
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtHosts));
        return DbMapErrorFromStmt(stmtHosts);
    }

    metadataHosts = mysql_stmt_result_metadata(stmtHosts);
    if (metadataHosts == NULL) {
        LOG_WARN("Unable to retrieve result metadata: %s", mysql_stmt_error(stmtHosts));
        return DbMapErrorFromStmt(stmtHosts);
    }

    //
    // Execute prepared admins statement and fetch results
    //

    result = mysql_stmt_execute(stmtAdmins);
    if (result != 0) {
        LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtAdmins));
        return DbMapErrorFromStmt(stmtAdmins);
    }

    DB_CHECK_RESULTS(bindOutputAdmins, metadataAdmins);
    ZeroMemory(&bindOutputAdmins, sizeof(bindOutputAdmins));

    // gname
    bindOutputAdmins[0].buffer_type   = MYSQL_TYPE_STRING;
    bindOutputAdmins[0].buffer        = buffer;
    bindOutputAdmins[0].buffer_length = sizeof(buffer);

    result = mysql_stmt_bind_result(stmtAdmins, bindOutputAdmins);
    if (result != 0) {
        LOG_WARN("Unable to bind results: %s", mysql_stmt_error(stmtAdmins));
        return DbMapErrorFromStmt(stmtAdmins);
    }

    result = mysql_stmt_store_result(stmtAdmins);
    if (result != 0) {
        LOG_WARN("Unable to buffer results: %s", mysql_stmt_error(stmtAdmins));
        return DbMapErrorFromStmt(stmtAdmins);
    }

    index = 0;
    while (mysql_stmt_fetch(stmtAdmins) == 0 && index < MAX_GROUPS) {

        // Resolve group names to IDs
        userFile->AdminGroups[index] = Io_Group2Gid(buffer);
        if (userFile->AdminGroups[index] == INVALID_GROUP) {
            LOG_WARN("Unable to resolve group \"%s\".", buffer);
        } else {
            index++;
        }
    }

    if (index < MAX_GROUPS) {
        // Terminate the admin groups array.
        userFile->AdminGroups[index] = INVALID_GROUP;
    }

    mysql_free_result(metadataAdmins);

    //
    // Execute prepared groups statement and fetch results
    //

    result = mysql_stmt_execute(stmtGroups);
    if (result != 0) {
        LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtGroups));
        return DbMapErrorFromStmt(stmtGroups);
    }

    DB_CHECK_RESULTS(bindOutputGroups, metadataGroups);
    ZeroMemory(&bindOutputGroups, sizeof(bindOutputGroups));

    // gname
    bindOutputGroups[0].buffer_type   = MYSQL_TYPE_STRING;
    bindOutputGroups[0].buffer        = buffer;
    bindOutputGroups[0].buffer_length = sizeof(buffer);

    result = mysql_stmt_bind_result(stmtGroups, bindOutputGroups);
    if (result != 0) {
        LOG_WARN("Unable to bind results: %s", mysql_stmt_error(stmtGroups));
        return DbMapErrorFromStmt(stmtGroups);
    }

    result = mysql_stmt_store_result(stmtGroups);
    if (result != 0) {
        LOG_WARN("Unable to buffer results: %s", mysql_stmt_error(stmtGroups));
        return DbMapErrorFromStmt(stmtGroups);
    }

    index = 0;
    while (mysql_stmt_fetch(stmtGroups) == 0 && index < MAX_GROUPS) {

        // Resolve group names to IDs
        userFile->Groups[index] = Io_Group2Gid(buffer);
        if (userFile->Groups[index] == INVALID_GROUP) {
            LOG_WARN("Unable to resolve group \"%s\".", buffer);
        } else {
            index++;
        }
    }

    if (index == 0) {
        // No groups, set to "NoGroup" and terminate the groups array.
        userFile->Groups[0] = NOGROUP_ID;
        userFile->Groups[1] = INVALID_GROUP;

    } else if (index < MAX_GROUPS) {
        // Terminate the groups array.
        userFile->Groups[index] = INVALID_GROUP;
    }

    mysql_free_result(metadataGroups);

    //
    // Execute prepared hosts statement and fetch results
    //

    result = mysql_stmt_execute(stmtHosts);
    if (result != 0) {
        LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtHosts));
        return DbMapErrorFromStmt(stmtHosts);
    }

    DB_CHECK_RESULTS(bindOutputHosts, metadataHosts);
    ZeroMemory(&bindOutputHosts, sizeof(bindOutputHosts));

    // host
    bindOutputHosts[0].buffer_type   = MYSQL_TYPE_STRING;
    bindOutputHosts[0].buffer        = buffer;
    bindOutputHosts[0].buffer_length = sizeof(buffer);

    result = mysql_stmt_bind_result(stmtHosts, bindOutputHosts);
    if (result != 0) {
        LOG_WARN("Unable to bind results: %s", mysql_stmt_error(stmtHosts));
        return DbMapErrorFromStmt(stmtHosts);
    }

    result = mysql_stmt_store_result(stmtHosts);
    if (result != 0) {
        LOG_WARN("Unable to buffer results: %s", mysql_stmt_error(stmtHosts));
        return DbMapErrorFromStmt(stmtHosts);
    }

    index = 0;
    while (mysql_stmt_fetch(stmtHosts) == 0 && index < MAX_IPS) {
        StringCchCopy(userFile->Ip[index], ELEMENT_COUNT(userFile->Ip[0]), buffer);
        index++;
    }

    mysql_free_result(metadataHosts);

    return ERROR_SUCCESS;
}

DWORD DbUserRead(DB_CONTEXT *db, CHAR *userName, USERFILE *userFilePtr)
{
    CHAR        *query;
    DWORD       error;
    INT         result;
    SIZE_T      userNameLength;
    USERFILE    userFile;
    MYSQL_BIND  bindInput[1];
    MYSQL_BIND  bindOutput[16];
    MYSQL_STMT  *stmt;
    MYSQL_RES   *metadata;

    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    ASSERT(userFilePtr != NULL);
    TRACE("db=%p userName=%s userFilePtr=%p", db, userName, userFilePtr);

    // Update a local copy of the user-file instead of the actual
    // user-file in case there is an error occurs before the update
    // is completely finished.
    ZeroMemory(&userFile, sizeof(USERFILE));

    stmt = db->stmt[0];

    userNameLength = strlen(userName);

    //
    // Prepare statement and bind parameters
    //

    query = "SELECT description,flags,home,limits,password,vfsfile,credits,"
            "       ratio,alldn,allup,daydn,dayup,monthdn,monthup,wkdn,wkup"
            "  FROM io_user"
            "  WHERE name=?";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    DB_CHECK_PARAMS(bindInput, stmt);
    ZeroMemory(&bindInput, sizeof(bindInput));

    // name=?
    bindInput[0].buffer_type   = MYSQL_TYPE_STRING;
    bindInput[0].buffer        = userName;
    bindInput[0].buffer_length = userNameLength;

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
    // Execute prepared statement and fetch results
    //

    result = mysql_stmt_execute(stmt);
    if (result != 0) {
        LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    DB_CHECK_RESULTS(bindOutput, metadata);
    ZeroMemory(&bindOutput, sizeof(bindOutput));

    // description
    bindOutput[0].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[0].buffer        = userFile.Tagline;
    bindOutput[0].buffer_length = sizeof(userFile.Tagline);

    // flags
    bindOutput[1].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[1].buffer        = userFile.Flags;
    bindOutput[1].buffer_length = sizeof(userFile.Flags);

    // home
    bindOutput[2].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[2].buffer        = userFile.Home;
    bindOutput[2].buffer_length = sizeof(userFile.Home);

    // limits
    bindOutput[3].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[3].buffer        = &userFile.Limits;
    bindOutput[3].buffer_length = sizeof(userFile.Limits);

    // password
    bindOutput[4].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[4].buffer        = &userFile.Password;
    bindOutput[4].buffer_length = sizeof(userFile.Password);

    // vfsfile
    bindOutput[5].buffer_type   = MYSQL_TYPE_STRING;
    bindOutput[5].buffer        = userFile.MountFile;
    bindOutput[5].buffer_length = sizeof(userFile.MountFile);

    // credits
    bindOutput[6].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[6].buffer        = &userFile.Credits;
    bindOutput[6].buffer_length = sizeof(userFile.Credits);

    // ratio
    bindOutput[7].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[7].buffer        = &userFile.Ratio;
    bindOutput[7].buffer_length = sizeof(userFile.Ratio);

    // alldn
    bindOutput[8].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[8].buffer        = &userFile.AllDn;
    bindOutput[8].buffer_length = sizeof(userFile.AllDn);

    // allup
    bindOutput[9].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[9].buffer        = &userFile.AllUp;
    bindOutput[9].buffer_length = sizeof(userFile.AllUp);

    // daydn
    bindOutput[10].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[10].buffer        = &userFile.DayDn;
    bindOutput[10].buffer_length = sizeof(userFile.DayDn);

    // dayup
    bindOutput[11].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[11].buffer        = &userFile.DayUp;
    bindOutput[11].buffer_length = sizeof(userFile.DayUp);

    // monthdn
    bindOutput[12].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[12].buffer        = &userFile.MonthDn;
    bindOutput[12].buffer_length = sizeof(userFile.MonthDn);

    // monthup
    bindOutput[13].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[13].buffer        = &userFile.MonthUp;
    bindOutput[13].buffer_length = sizeof(userFile.MonthUp);

    // wkdn
    bindOutput[14].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[14].buffer        = &userFile.WkDn;
    bindOutput[14].buffer_length = sizeof(userFile.WkDn);

    // wkup
    bindOutput[15].buffer_type   = MYSQL_TYPE_BLOB;
    bindOutput[15].buffer        = &userFile.WkUp;
    bindOutput[15].buffer_length = sizeof(userFile.WkUp);

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

    result = mysql_stmt_fetch(stmt);
    if (result != 0) {
        LOG_WARN("Unable to fetch results: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    mysql_free_result(metadata);

    //
    // Read the user's admin-groups, groups, and hosts.
    //

    error = DbUserReadExtra(db, userName, &userFile);
    if (error != ERROR_SUCCESS) {
        LOG_WARN("Unable to read additional user data (error %lu).", error);
        return error;
    }

    //
    // Initialize remaining values of the user-file structure and copy the
    // local user-file to the output parameter. Copy all structure members up
    // to lpInternal, the lpInternal and lpParent members must not be changed.
    //
    userFile.Uid = userFilePtr->Uid;
    userFile.Gid = userFile.Groups[0];

    CopyMemory(userFilePtr, &userFile, offsetof(USERFILE, lpInternal));

    return ERROR_SUCCESS;
}

DWORD DbUserCreate(DB_CONTEXT *db, CHAR *userName, USERFILE *userFile)
{
    BYTE        syncEvent;
    CHAR        buffer[128];
    CHAR        *host;
    CHAR        *query;
    DWORD       error;
    INT         groupId;
    INT         i;
    INT         result;
    SIZE_T      bufferLength;
    SIZE_T      userNameLength;
    MYSQL_BIND  bindAdmins[2];
    MYSQL_BIND  bindChanges[2];
    MYSQL_BIND  bindGroups[3];
    MYSQL_BIND  bindHosts[2];
    MYSQL_BIND  bindUsers[17];
    MYSQL_STMT  *stmtAdmins;
    MYSQL_STMT  *stmtChanges;
    MYSQL_STMT  *stmtGroups;
    MYSQL_STMT  *stmtHosts;
    MYSQL_STMT  *stmtUsers;

    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    TRACE("db=%p userName=%s userFile=%p", db, userName, userFile);

    // Buffer must be large enough to hold a group-name or host-mask.
    ASSERT(sizeof(buffer) > _IP_LINE_LENGTH);
    ASSERT(sizeof(buffer) > _MAX_NAME);
    ZeroMemory(buffer, sizeof(buffer));

    stmtUsers   = db->stmt[0];
    stmtAdmins  = db->stmt[1];
    stmtGroups  = db->stmt[2];
    stmtHosts   = db->stmt[3];
    stmtChanges = db->stmt[4];

    userNameLength = strlen(userName);

    //
    // Prepare users statement and bind parameters
    //

    query = "INSERT INTO io_user"
            "(name,description,flags,home,limits,password,vfsfile,credits,"
            "ratio,alldn,allup,daydn,dayup,monthdn,monthup,wkdn,wkup)"
            " VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";

    result = mysql_stmt_prepare(stmtUsers, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtUsers));
        return DbMapErrorFromStmt(stmtUsers);
    }

    DB_CHECK_PARAMS(bindUsers, stmtUsers);
    ZeroMemory(&bindUsers, sizeof(bindUsers));

    // name
    bindUsers[0].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[0].buffer        = userName;
    bindUsers[0].buffer_length = userNameLength;

    // description
    bindUsers[1].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[1].buffer        = userFile->Tagline;
    bindUsers[1].buffer_length = strlen(userFile->Tagline);

    // flags
    bindUsers[2].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[2].buffer        = userFile->Flags;
    bindUsers[2].buffer_length = strlen(userFile->Flags);

    // home
    bindUsers[3].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[3].buffer        = userFile->Home;
    bindUsers[3].buffer_length = strlen(userFile->Home);

    // limits
    bindUsers[4].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[4].buffer        = &userFile->Limits;
    bindUsers[4].buffer_length = sizeof(userFile->Limits);

    // password
    bindUsers[5].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[5].buffer        = &userFile->Password;
    bindUsers[5].buffer_length = sizeof(userFile->Password);

    // vfsfile
    bindUsers[6].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[6].buffer        = userFile->MountFile;
    bindUsers[6].buffer_length = strlen(userFile->MountFile);

    // credits
    bindUsers[7].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[7].buffer        = &userFile->Credits;
    bindUsers[7].buffer_length = sizeof(userFile->Credits);

    // ratio
    bindUsers[8].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[8].buffer        = &userFile->Ratio;
    bindUsers[8].buffer_length = sizeof(userFile->Ratio);

    // alldn=?
    bindUsers[9].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[9].buffer        = &userFile->AllDn;
    bindUsers[9].buffer_length = sizeof(userFile->AllDn);

    // allup=?
    bindUsers[10].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[10].buffer        = &userFile->AllUp;
    bindUsers[10].buffer_length = sizeof(userFile->AllUp);

    // daydn=?
    bindUsers[11].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[11].buffer        = &userFile->DayDn;
    bindUsers[11].buffer_length = sizeof(userFile->DayDn);

    // dayup=?
    bindUsers[12].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[12].buffer        = &userFile->DayUp;
    bindUsers[12].buffer_length = sizeof(userFile->DayUp);

    // monthdn=?
    bindUsers[13].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[13].buffer        = &userFile->MonthDn;
    bindUsers[13].buffer_length = sizeof(userFile->MonthDn);

    // monthup=?
    bindUsers[14].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[14].buffer        = &userFile->MonthUp;
    bindUsers[14].buffer_length = sizeof(userFile->MonthUp);

    // wkdn=?
    bindUsers[15].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[15].buffer        = &userFile->WkDn;
    bindUsers[15].buffer_length = sizeof(userFile->WkDn);

    // wkup=?
    bindUsers[16].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[16].buffer        = &userFile->WkUp;
    bindUsers[16].buffer_length = sizeof(userFile->WkUp);

    result = mysql_stmt_bind_param(stmtUsers, bindUsers);
    if (result != 0) {
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtUsers));
        return DbMapErrorFromStmt(stmtUsers);
    }

    //
    // Prepare admins statement and bind parameters
    //

    query = "REPLACE INTO io_user_admins(uname,gname) VALUES(?,?)";

    result = mysql_stmt_prepare(stmtAdmins, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtAdmins));
        return DbMapErrorFromStmt(stmtAdmins);
    }

    DB_CHECK_PARAMS(bindAdmins, stmtAdmins);
    ZeroMemory(&bindAdmins, sizeof(bindAdmins));

    bindAdmins[0].buffer_type   = MYSQL_TYPE_STRING;
    bindAdmins[0].buffer        = userName;
    bindAdmins[0].buffer_length = userNameLength;

    bindAdmins[1].buffer_type   = MYSQL_TYPE_STRING;
    bindAdmins[1].buffer        = &buffer;
    bindAdmins[1].buffer_length = _MAX_NAME;
    bindAdmins[1].length        = &bufferLength;

    result = mysql_stmt_bind_param(stmtAdmins, bindAdmins);
    if (result != 0) {
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtAdmins));
        return DbMapErrorFromStmt(stmtAdmins);
    }

    //
    // Prepare groups statement and bind parameters
    //

    query = "REPLACE INTO io_user_groups(uname,gname,idx) VALUES(?,?,?)";

    result = mysql_stmt_prepare(stmtGroups, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtGroups));
        return DbMapErrorFromStmt(stmtGroups);
    }

    DB_CHECK_PARAMS(bindGroups, stmtGroups);
    ZeroMemory(&bindGroups, sizeof(bindGroups));

    bindGroups[0].buffer_type   = MYSQL_TYPE_STRING;
    bindGroups[0].buffer        = userName;
    bindGroups[0].buffer_length = userNameLength;

    bindGroups[1].buffer_type   = MYSQL_TYPE_STRING;
    bindGroups[1].buffer        = &buffer;
    bindGroups[1].buffer_length = _MAX_NAME;
    bindGroups[1].length        = &bufferLength;

    bindGroups[2].buffer_type   = MYSQL_TYPE_LONG;
    bindGroups[2].buffer        = &i;

    result = mysql_stmt_bind_param(stmtGroups, bindGroups);
    if (result != 0) {
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtGroups));
        return DbMapErrorFromStmt(stmtGroups);
    }

    //
    // Prepare hosts statement and bind parameters
    //

    query = "REPLACE INTO io_user_hosts(uname,host) VALUES(?,?)";

    result = mysql_stmt_prepare(stmtHosts, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtHosts));
        return DbMapErrorFromStmt(stmtHosts);
    }

    DB_CHECK_PARAMS(bindHosts, stmtHosts);
    ZeroMemory(&bindHosts, sizeof(bindHosts));

    bindHosts[0].buffer_type   = MYSQL_TYPE_STRING;
    bindHosts[0].buffer        = userName;
    bindHosts[0].buffer_length = userNameLength;

    bindHosts[1].buffer_type   = MYSQL_TYPE_STRING;
    bindHosts[1].buffer        = &buffer;
    bindHosts[1].buffer_length = _IP_LINE_LENGTH;
    bindHosts[1].length        = &bufferLength;

    result = mysql_stmt_bind_param(stmtHosts, bindHosts);
    if (result != 0) {
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtHosts));
        return DbMapErrorFromStmt(stmtHosts);
    }

    //
    // Prepare changes statement and bind parameters
    //

    query = "INSERT INTO io_user_changes"
            " (time,type,name)"
            " VALUES(UNIX_TIMESTAMP(),?,?)";

    result = mysql_stmt_prepare(stmtChanges, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtChanges));
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
    bindChanges[1].buffer        = userName;
    bindChanges[1].buffer_length = userNameLength;

    result = mysql_stmt_bind_param(stmtChanges, bindChanges);
    if (result != 0) {
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtChanges));
        return DbMapErrorFromStmt(stmtChanges);
    }

    //
    // Begin transaction
    //

    result = mysql_query(db->handle, "START TRANSACTION");
    if (result != 0) {
        LOG_WARN("Unable to start transaction: %s", mysql_error(db->handle));
        return DbMapErrorFromConn(db->handle);
    }

    //
    // Execute prepared statements
    //

    result = mysql_stmt_execute(stmtUsers);
    if (result != 0) {
        LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtUsers));
        error = DbMapErrorFromStmt(stmtUsers);
        goto rollback;
    }

    for (i = 0; i < MAX_GROUPS; i++) {
        groupId = userFile->AdminGroups[i];
        if (groupId == INVALID_GROUP) {
            break;
        }
        if (CopyGroup(groupId, buffer, ELEMENT_COUNT(buffer), &bufferLength)) {
            result = mysql_stmt_execute(stmtAdmins);
            if (result != 0) {
                LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtAdmins));
                error = DbMapErrorFromStmt(stmtAdmins);
                goto rollback;
            }
        }
    }

    for (i = 0; i < MAX_GROUPS; i++) {
        groupId = userFile->Groups[i];
        if (groupId == INVALID_GROUP) {
            break;
        }
        if (CopyGroup(groupId, buffer, ELEMENT_COUNT(buffer), &bufferLength)) {
            result = mysql_stmt_execute(stmtGroups);
            if (result != 0) {
                LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtGroups));
                error = DbMapErrorFromStmt(stmtGroups);
                goto rollback;
            }
        }
    }

    for (i = 0; i < MAX_IPS; i++) {
        host = userFile->Ip[i];
        if (host[0] == '\0') {
            break;
        }
        CopyHost(host, buffer, ELEMENT_COUNT(buffer), &bufferLength);

        result = mysql_stmt_execute(stmtHosts);
        if (result != 0) {
            LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtHosts));
            error = DbMapErrorFromStmt(stmtHosts);
            goto rollback;
        }
    }

    result = mysql_stmt_execute(stmtChanges);
    if (result != 0) {
        LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtChanges));
        error = DbMapErrorFromStmt(stmtChanges);
        goto rollback;
    }

    //
    // Commit transaction
    //

    result = mysql_query(db->handle, "COMMIT");
    if (result != 0) {
        LOG_WARN("Unable to commit transaction: %s", mysql_error(db->handle));
        return DbMapErrorFromConn(db->handle);
    }

    return ERROR_SUCCESS;

rollback:
    //
    // Rollback transaction on error
    //

    if (mysql_query(db->handle, "ROLLBACK") != 0) {
        LOG_WARN("Unable to rollback transaction: %s", mysql_error(db->handle));
    }

    ASSERT(error != ERROR_SUCCESS);
    return error;
}

DWORD DbUserRename(DB_CONTEXT *db, CHAR *userName, CHAR *newName)
{
    BYTE        syncEvent;
    CHAR        *query;
    DWORD       error;
    INT         result;
    INT64       affectedRows;
    SIZE_T      userNameLength;
    SIZE_T      newNameLength;
    MYSQL_BIND  bindAdmins[2];
    MYSQL_BIND  bindChanges[3];
    MYSQL_BIND  bindGroups[2];
    MYSQL_BIND  bindHosts[2];
    MYSQL_BIND  bindUsers[2];
    MYSQL_STMT  *stmtAdmins;
    MYSQL_STMT  *stmtChanges;
    MYSQL_STMT  *stmtGroups;
    MYSQL_STMT  *stmtHosts;
    MYSQL_STMT  *stmtUsers;

    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    ASSERT(newName != NULL);
    TRACE("db=%p userName=%s newName=%s", db, userName, newName);

    stmtUsers   = db->stmt[0];
    stmtAdmins  = db->stmt[1];
    stmtGroups  = db->stmt[2];
    stmtHosts   = db->stmt[3];
    stmtChanges = db->stmt[4];

    userNameLength = strlen(userName);
    newNameLength  = strlen(newName);

    //
    // Prepare users statement and bind parameters
    //

    query = "UPDATE io_user SET name=? WHERE name=?";

    result = mysql_stmt_prepare(stmtUsers, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtUsers));
        return DbMapErrorFromStmt(stmtUsers);
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
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtUsers));
        return DbMapErrorFromStmt(stmtUsers);
    }

    //
    // Prepare admins statement and bind parameters
    //

    query = "UPDATE io_user_admins SET uname=? WHERE uname=?";

    result = mysql_stmt_prepare(stmtAdmins, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtAdmins));
        return DbMapErrorFromStmt(stmtAdmins);
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
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtAdmins));
        return DbMapErrorFromStmt(stmtAdmins);
    }

    //
    // Prepare groups statement and bind parameters
    //

    query = "UPDATE io_user_groups SET uname=? WHERE uname=?";

    result = mysql_stmt_prepare(stmtGroups, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtGroups));
        return DbMapErrorFromStmt(stmtGroups);
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
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtGroups));
        return DbMapErrorFromStmt(stmtGroups);
    }

    //
    // Prepare hosts statement and bind parameters
    //

    query = "UPDATE io_user_hosts SET uname=? WHERE uname=?";

    result = mysql_stmt_prepare(stmtHosts, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtHosts));
        return DbMapErrorFromStmt(stmtHosts);
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
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtHosts));
        return DbMapErrorFromStmt(stmtHosts);
    }

    //
    // Prepare changes statement and bind parameters
    //

    query = "INSERT INTO io_user_changes"
            " (time,type,name,info)"
            " VALUES(UNIX_TIMESTAMP(),?,?,?)";

    result = mysql_stmt_prepare(stmtChanges, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtChanges));
        return DbMapErrorFromStmt(stmtChanges);
    }

    DB_CHECK_PARAMS(bindChanges, stmtChanges);
    ZeroMemory(&bindChanges, sizeof(bindChanges));

    // Change event used during incremental syncs.
    syncEvent = SYNC_EVENT_RENAME;

    bindChanges[0].buffer_type   = MYSQL_TYPE_TINY;
    bindChanges[0].buffer        = &syncEvent;
    bindChanges[0].is_unsigned   = TRUE;

    bindChanges[1].buffer_type   = MYSQL_TYPE_STRING;
    bindChanges[1].buffer        = userName;
    bindChanges[1].buffer_length = userNameLength;

    bindChanges[2].buffer_type   = MYSQL_TYPE_STRING;
    bindChanges[2].buffer        = newName;
    bindChanges[2].buffer_length = newNameLength;

    result = mysql_stmt_bind_param(stmtChanges, bindChanges);
    if (result != 0) {
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtChanges));
        return DbMapErrorFromStmt(stmtChanges);
    }

    //
    // Begin transaction
    //

    result = mysql_query(db->handle, "START TRANSACTION");
    if (result != 0) {
        LOG_WARN("Unable to start transaction: %s", mysql_error(db->handle));
        return DbMapErrorFromConn(db->handle);
    }

    //
    // Execute prepared statements
    //

    result = mysql_stmt_execute(stmtUsers);
    if (result != 0) {
        LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtUsers));
        error = DbMapErrorFromStmt(stmtUsers);
        goto rollback;
    }

    result = mysql_stmt_execute(stmtAdmins);
    if (result != 0) {
        LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtAdmins));
        error = DbMapErrorFromStmt(stmtAdmins);
        goto rollback;
    }

    result = mysql_stmt_execute(stmtGroups);
    if (result != 0) {
        LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtGroups));
        error = DbMapErrorFromStmt(stmtGroups);
        goto rollback;
    }

    result = mysql_stmt_execute(stmtHosts);
    if (result != 0) {
        LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtHosts));
        error = DbMapErrorFromStmt(stmtHosts);
        goto rollback;
    }

    affectedRows = mysql_stmt_affected_rows(stmtUsers);
    if (affectedRows > 0) {

        // Only insert a changes event if the group was deleted.
        result = mysql_stmt_execute(stmtChanges);
        if (result != 0) {
            LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtChanges));
            error = DbMapErrorFromStmt(stmtChanges);
            goto rollback;
        }
    }

    //
    // Commit transaction
    //

    result = mysql_query(db->handle, "COMMIT");
    if (result != 0) {
        LOG_WARN("Unable to commit transaction: %s", mysql_error(db->handle));
        return DbMapErrorFromConn(db->handle);
    }

    //
    // Check for modified rows
    //

    if (affectedRows == 0) {
        LOG_WARN("Unable to rename user: no affected rows");
        return ERROR_USER_NOT_FOUND;
    }

    return ERROR_SUCCESS;

rollback:
    //
    // Rollback transaction on error
    //

    if (mysql_query(db->handle, "ROLLBACK") != 0) {
        LOG_WARN("Unable to rollback transaction: %s", mysql_error(db->handle));
    }

    ASSERT(error != ERROR_SUCCESS);
    return error;
}

DWORD DbUserDelete(DB_CONTEXT *db, CHAR *userName)
{
    BYTE        syncEvent;
    CHAR        *query;
    DWORD       error;
    INT         result;
    INT64       affectedRows;
    SIZE_T      userNameLength;
    MYSQL_BIND  bindAdmins[1];
    MYSQL_BIND  bindChanges[2];
    MYSQL_BIND  bindGroups[1];
    MYSQL_BIND  bindHosts[1];
    MYSQL_BIND  bindUsers[1];
    MYSQL_STMT  *stmtAdmins;
    MYSQL_STMT  *stmtChanges;
    MYSQL_STMT  *stmtGroups;
    MYSQL_STMT  *stmtHosts;
    MYSQL_STMT  *stmtUsers;

    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    TRACE("db=%p userName=%s", db, userName);

    stmtUsers   = db->stmt[0];
    stmtAdmins  = db->stmt[1];
    stmtGroups  = db->stmt[2];
    stmtHosts   = db->stmt[3];
    stmtChanges = db->stmt[4];

    userNameLength = strlen(userName);

    //
    // Prepare users statement and bind parameters
    //

    query = "DELETE FROM io_user WHERE name=?";

    result = mysql_stmt_prepare(stmtUsers, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtUsers));
        return DbMapErrorFromStmt(stmtUsers);
    }

    DB_CHECK_PARAMS(bindUsers, stmtUsers);
    ZeroMemory(&bindUsers, sizeof(bindUsers));

    bindUsers[0].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[0].buffer        = userName;
    bindUsers[0].buffer_length = userNameLength;

    result = mysql_stmt_bind_param(stmtUsers, bindUsers);
    if (result != 0) {
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtUsers));
        return DbMapErrorFromStmt(stmtUsers);
    }

    //
    // Prepare admins statement and bind parameters
    //

    query = "DELETE FROM io_user_admins WHERE uname=?";

    result = mysql_stmt_prepare(stmtAdmins, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtAdmins));
        return DbMapErrorFromStmt(stmtAdmins);
    }

    DB_CHECK_PARAMS(bindAdmins, stmtAdmins);
    ZeroMemory(&bindAdmins, sizeof(bindAdmins));

    bindAdmins[0].buffer_type   = MYSQL_TYPE_STRING;
    bindAdmins[0].buffer        = userName;
    bindAdmins[0].buffer_length = userNameLength;

    result = mysql_stmt_bind_param(stmtAdmins, bindAdmins);
    if (result != 0) {
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtAdmins));
        return DbMapErrorFromStmt(stmtAdmins);
    }

    //
    // Prepare groups statement and bind parameters
    //

    query = "DELETE FROM io_user_groups WHERE uname=?";

    result = mysql_stmt_prepare(stmtGroups, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtGroups));
        return DbMapErrorFromStmt(stmtGroups);
    }

    DB_CHECK_PARAMS(bindGroups, stmtGroups);
    ZeroMemory(&bindGroups, sizeof(bindGroups));

    bindGroups[0].buffer_type   = MYSQL_TYPE_STRING;
    bindGroups[0].buffer        = userName;
    bindGroups[0].buffer_length = userNameLength;

    result = mysql_stmt_bind_param(stmtGroups, bindGroups);
    if (result != 0) {
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtGroups));
        return DbMapErrorFromStmt(stmtGroups);
    }

    //
    // Prepare hosts statement and bind parameters
    //

    query = "DELETE FROM io_user_hosts WHERE uname=?";

    result = mysql_stmt_prepare(stmtHosts, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtHosts));
        return DbMapErrorFromStmt(stmtHosts);
    }

    DB_CHECK_PARAMS(bindHosts, stmtHosts);
    ZeroMemory(&bindHosts, sizeof(bindHosts));

    bindHosts[0].buffer_type   = MYSQL_TYPE_STRING;
    bindHosts[0].buffer        = userName;
    bindHosts[0].buffer_length = userNameLength;

    result = mysql_stmt_bind_param(stmtHosts, bindHosts);
    if (result != 0) {
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtHosts));
        return DbMapErrorFromStmt(stmtHosts);
    }

    //
    // Prepare changes statement and bind parameters
    //

    query = "INSERT INTO io_user_changes"
            " (time,type,name)"
            " VALUES(UNIX_TIMESTAMP(),?,?)";

    result = mysql_stmt_prepare(stmtChanges, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtChanges));
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
    bindChanges[1].buffer        = userName;
    bindChanges[1].buffer_length = userNameLength;

    result = mysql_stmt_bind_param(stmtChanges, bindChanges);
    if (result != 0) {
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtChanges));
        return DbMapErrorFromStmt(stmtChanges);
    }

    //
    // Begin transaction
    //

    result = mysql_query(db->handle, "START TRANSACTION");
    if (result != 0) {
        LOG_WARN("Unable to start transaction: %s", mysql_error(db->handle));
        return DbMapErrorFromConn(db->handle);
    }

    //
    // Execute prepared statements
    //

    result = mysql_stmt_execute(stmtUsers);
    if (result != 0) {
        LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtUsers));
        error = DbMapErrorFromStmt(stmtUsers);
        goto rollback;
    }

    result = mysql_stmt_execute(stmtAdmins);
    if (result != 0) {
        LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtAdmins));
        error = DbMapErrorFromStmt(stmtAdmins);
        goto rollback;
    }

    result = mysql_stmt_execute(stmtGroups);
    if (result != 0) {
        LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtGroups));
        error = DbMapErrorFromStmt(stmtGroups);
        goto rollback;
    }

    result = mysql_stmt_execute(stmtHosts);
    if (result != 0) {
        LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtHosts));
        error = DbMapErrorFromStmt(stmtHosts);
        goto rollback;
    }

    affectedRows = mysql_stmt_affected_rows(stmtUsers);
    if (affectedRows > 0) {

        // Only insert a changes event if the group was deleted.
        result = mysql_stmt_execute(stmtChanges);
        if (result != 0) {
            LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtChanges));
            error = DbMapErrorFromStmt(stmtChanges);
            goto rollback;
        }
    }

    //
    // Commit transaction
    //

    result = mysql_query(db->handle, "COMMIT");
    if (result != 0) {
        LOG_WARN("Unable to commit transaction: %s", mysql_error(db->handle));
        return DbMapErrorFromConn(db->handle);
    }

    //
    // Check for deleted rows
    //

    if (affectedRows == 0) {
        LOG_WARN("Unable to delete user: no affected rows");
        return ERROR_USER_NOT_FOUND;
    }

    return ERROR_SUCCESS;

rollback:
    //
    // Rollback transaction on error
    //

    if (mysql_query(db->handle, "ROLLBACK") != 0) {
        LOG_WARN("Unable to rollback transaction: %s", mysql_error(db->handle));
    }

    ASSERT(error != ERROR_SUCCESS);
    return error;
}

DWORD DbUserLock(DB_CONTEXT *db, CHAR *userName, USERFILE *userFile)
{
    CHAR        *query;
    DWORD       error;
    INT         result;
    INT64       affectedRows;
    MYSQL_BIND  bind[4];
    MYSQL_STMT  *stmt;

    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    TRACE("db=%p userName=%s userFile=%p", db, userName, userFile);

    stmt = db->stmt[0];

    //
    // Prepare statement and bind parameters
    //

    // Parameters: user, expire, timeout, owner
    query = "CALL io_user_lock(?,?,?,?)";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    DB_CHECK_PARAMS(bind, stmt);
    ZeroMemory(&bind, sizeof(bind));

    bind[0].buffer_type   = MYSQL_TYPE_STRING;
    bind[0].buffer        = userName;
    bind[0].buffer_length = strlen(userName);

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

    //
    // Check for modified rows
    //

    affectedRows = mysql_stmt_affected_rows(stmt);
    if (affectedRows == 0) {
        LOG_WARN("Unable to lock user: no affected rows");
        return ERROR_USER_LOCK_FAILED;
    }

    ASSERT(affectedRows == 1);

    //
    // Update user data
    //

    error = DbUserRead(db, userName, userFile);
    if (error != ERROR_SUCCESS) {
        LOG_WARN("Unable to update user \"%s\" on lock (error %lu).", userName, error);
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
    TRACE("db=%p userName=%s", db, userName);

    stmt = db->stmt[0];

    //
    // Prepare statement and bind parameters
    //

    query = "UPDATE io_user SET lockowner=NULL, locktime=0"
            "  WHERE name=? AND lockowner=?";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
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

    affectedRows = mysql_stmt_affected_rows(stmt);
    if (affectedRows == 0) {
        // Failure is acceptable
        LOG_WARN("Unable to unlock user: no affected rows");
    }

    return ERROR_SUCCESS;
}

DWORD DbUserOpen(DB_CONTEXT *db, CHAR *userName, USERFILE *userFile)
{
    ASSERT(db != NULL);
    ASSERT(userName != NULL);
    ASSERT(userFile != NULL);
    TRACE("db=%p userName=%s userFile=%p", db, userName, userFile);

    return DbUserRead(db, userName, userFile);
}

DWORD DbUserWrite(DB_CONTEXT *db, CHAR *userName, USERFILE *userFile)
{
    CHAR        buffer[128];
    CHAR        *host;
    CHAR        *query;
    DWORD       error;
    INT         groupId;
    INT         i;
    INT         result;
    SIZE_T      bufferLength;
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
    TRACE("db=%p userName=%s userFile=%p", db, userName, userFile);

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

    query = "UPDATE io_user SET description=?, flags=?, home=?, limits=?,"
            " password=?, vfsfile=?, credits=?, ratio=?, alldn=?, allup=?,"
            " daydn=?, dayup=?, monthdn=?, monthup=?, wkdn=?, wkup=?,"
            " updated=UNIX_TIMESTAMP()"
            "   WHERE name=?";

    result = mysql_stmt_prepare(stmtUsers, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtUsers));
        return DbMapErrorFromStmt(stmtUsers);
    }

    DB_CHECK_PARAMS(bindUsers, stmtUsers);
    ZeroMemory(&bindUsers, sizeof(bindUsers));

    // description=?
    bindUsers[0].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[0].buffer        = userFile->Tagline;
    bindUsers[0].buffer_length = strlen(userFile->Tagline);

    // flags=?
    bindUsers[1].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[1].buffer        = userFile->Flags;
    bindUsers[1].buffer_length = strlen(userFile->Flags);

    // home=?
    bindUsers[2].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[2].buffer        = userFile->Home;
    bindUsers[2].buffer_length = strlen(userFile->Home);

    // limits=?
    bindUsers[3].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[3].buffer        = &userFile->Limits;
    bindUsers[3].buffer_length = sizeof(userFile->Limits);

    // password=?
    bindUsers[4].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[4].buffer        = &userFile->Password;
    bindUsers[4].buffer_length = sizeof(userFile->Password);

    // vfsfile=?
    bindUsers[5].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[5].buffer        = userFile->MountFile;
    bindUsers[5].buffer_length = strlen(userFile->MountFile);

    // credits=?
    bindUsers[6].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[6].buffer        = &userFile->Credits;
    bindUsers[6].buffer_length = sizeof(userFile->Credits);

    // ratio=?
    bindUsers[7].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[7].buffer        = &userFile->Ratio;
    bindUsers[7].buffer_length = sizeof(userFile->Ratio);

    // alldn=?
    bindUsers[8].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[8].buffer        = &userFile->AllDn;
    bindUsers[8].buffer_length = sizeof(userFile->AllDn);

    // allup=?
    bindUsers[9].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[9].buffer        = &userFile->AllUp;
    bindUsers[9].buffer_length = sizeof(userFile->AllUp);

    // daydn=?
    bindUsers[10].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[10].buffer        = &userFile->DayDn;
    bindUsers[10].buffer_length = sizeof(userFile->DayDn);

    // dayup=?
    bindUsers[11].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[11].buffer        = &userFile->DayUp;
    bindUsers[11].buffer_length = sizeof(userFile->DayUp);

    // monthdn=?
    bindUsers[12].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[12].buffer        = &userFile->MonthDn;
    bindUsers[12].buffer_length = sizeof(userFile->MonthDn);

    // monthup=?
    bindUsers[13].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[13].buffer        = &userFile->MonthUp;
    bindUsers[13].buffer_length = sizeof(userFile->MonthUp);

    // wkdn=?
    bindUsers[14].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[14].buffer        = &userFile->WkDn;
    bindUsers[14].buffer_length = sizeof(userFile->WkDn);

    // wkup=?
    bindUsers[15].buffer_type   = MYSQL_TYPE_BLOB;
    bindUsers[15].buffer        = &userFile->WkUp;
    bindUsers[15].buffer_length = sizeof(userFile->WkUp);

    // name=?
    bindUsers[16].buffer_type   = MYSQL_TYPE_STRING;
    bindUsers[16].buffer        = userName;
    bindUsers[16].buffer_length = userNameLength;

    result = mysql_stmt_bind_param(stmtUsers, bindUsers);
    if (result != 0) {
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtUsers));
        return DbMapErrorFromStmt(stmtUsers);
    }

    //
    // Prepare admins statement and bind parameters
    //

    query = "REPLACE INTO io_user_admins(uname,gname) VALUES(?,?)";

    result = mysql_stmt_prepare(stmtAddAdmins, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtAddAdmins));
        return DbMapErrorFromStmt(stmtAddAdmins);
    }

    DB_CHECK_PARAMS(bindAddAdmins, stmtAddAdmins);
    ZeroMemory(&bindAddAdmins, sizeof(bindAddAdmins));

    bindAddAdmins[0].buffer_type   = MYSQL_TYPE_STRING;
    bindAddAdmins[0].buffer        = userName;
    bindAddAdmins[0].buffer_length = userNameLength;

    bindAddAdmins[1].buffer_type   = MYSQL_TYPE_STRING;
    bindAddAdmins[1].buffer        = &buffer;
    bindAddAdmins[1].buffer_length = _MAX_NAME;
    bindAddAdmins[1].length        = &bufferLength;

    result = mysql_stmt_bind_param(stmtAddAdmins, bindAddAdmins);
    if (result != 0) {
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtAddAdmins));
        return DbMapErrorFromStmt(stmtAddAdmins);
    }

    //
    // Prepare groups statement and bind parameters
    //

    query = "REPLACE INTO io_user_groups(uname,gname,idx) VALUES(?,?,?)";

    result = mysql_stmt_prepare(stmtAddGroups, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtAddGroups));
        return DbMapErrorFromStmt(stmtAddGroups);
    }

    DB_CHECK_PARAMS(bindAddGroups, stmtAddGroups);
    ZeroMemory(&bindAddGroups, sizeof(bindAddGroups));

    bindAddGroups[0].buffer_type   = MYSQL_TYPE_STRING;
    bindAddGroups[0].buffer        = userName;
    bindAddGroups[0].buffer_length = userNameLength;

    bindAddGroups[1].buffer_type   = MYSQL_TYPE_STRING;
    bindAddGroups[1].buffer        = &buffer;
    bindAddGroups[1].buffer_length = _MAX_NAME;
    bindAddGroups[1].length        = &bufferLength;

    bindAddGroups[2].buffer_type   = MYSQL_TYPE_LONG;
    bindAddGroups[2].buffer        = &i;

    result = mysql_stmt_bind_param(stmtAddGroups, bindAddGroups);
    if (result != 0) {
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtAddGroups));
        return DbMapErrorFromStmt(stmtAddGroups);
    }

    //
    // Prepare hosts statement and bind parameters
    //

    query = "REPLACE INTO io_user_hosts(uname,host) VALUES(?,?)";

    result = mysql_stmt_prepare(stmtAddHosts, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtAddHosts));
        return DbMapErrorFromStmt(stmtAddHosts);
    }

    DB_CHECK_PARAMS(bindAddHosts, stmtAddHosts);
    ZeroMemory(&bindAddHosts, sizeof(bindAddHosts));

    bindAddHosts[0].buffer_type   = MYSQL_TYPE_STRING;
    bindAddHosts[0].buffer        = userName;
    bindAddHosts[0].buffer_length = userNameLength;

    bindAddHosts[1].buffer_type   = MYSQL_TYPE_STRING;
    bindAddHosts[1].buffer        = &buffer;
    bindAddHosts[1].buffer_length = _IP_LINE_LENGTH;
    bindAddHosts[1].length        = &bufferLength;

    result = mysql_stmt_bind_param(stmtAddHosts, bindAddHosts);
    if (result != 0) {
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtAddHosts));
        return DbMapErrorFromStmt(stmtAddHosts);
    }

    //
    // Prepare and bind admins delete statement
    //

    query = "DELETE FROM io_user_admins WHERE uname=?";

    result = mysql_stmt_prepare(stmtDelAdmins, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtDelAdmins));
        return DbMapErrorFromStmt(stmtDelAdmins);
    }

    DB_CHECK_PARAMS(bindDelAdmins, stmtDelAdmins);
    ZeroMemory(&bindDelAdmins, sizeof(bindDelAdmins));

    bindDelAdmins[0].buffer_type   = MYSQL_TYPE_STRING;
    bindDelAdmins[0].buffer        = userName;
    bindDelAdmins[0].buffer_length = userNameLength;

    result = mysql_stmt_bind_param(stmtDelAdmins, bindDelAdmins);
    if (result != 0) {
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtDelAdmins));
        return DbMapErrorFromStmt(stmtDelAdmins);
    }

    //
    // Prepare and bind groups delete statement
    //

    query = "DELETE FROM io_user_groups WHERE uname=?";

    result = mysql_stmt_prepare(stmtDelGroups, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtDelGroups));
        return DbMapErrorFromStmt(stmtDelGroups);
    }

    DB_CHECK_PARAMS(bindDelGroups, stmtDelGroups);
    ZeroMemory(&bindDelGroups, sizeof(bindDelGroups));

    bindDelGroups[0].buffer_type   = MYSQL_TYPE_STRING;
    bindDelGroups[0].buffer        = userName;
    bindDelGroups[0].buffer_length = userNameLength;

    result = mysql_stmt_bind_param(stmtDelGroups, bindDelGroups);
    if (result != 0) {
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtDelGroups));
        return DbMapErrorFromStmt(stmtDelGroups);
    }

    //
    // Prepare and bind hosts delete statement
    //

    query = "DELETE FROM io_user_hosts WHERE uname=?";

    result = mysql_stmt_prepare(stmtDelHosts, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmtDelHosts));
        return DbMapErrorFromStmt(stmtDelHosts);
    }

    DB_CHECK_PARAMS(bindDelHosts, stmtDelHosts);
    ZeroMemory(&bindDelHosts, sizeof(bindDelHosts));

    bindDelHosts[0].buffer_type   = MYSQL_TYPE_STRING;
    bindDelHosts[0].buffer        = userName;
    bindDelHosts[0].buffer_length = userNameLength;

    result = mysql_stmt_bind_param(stmtDelHosts, bindDelHosts);
    if (result != 0) {
        LOG_WARN("Unable to bind parameters: %s", mysql_stmt_error(stmtDelHosts));
        return DbMapErrorFromStmt(stmtDelHosts);
    }

    //
    // Begin transaction
    //

    result = mysql_query(db->handle, "START TRANSACTION");
    if (result != 0) {
        LOG_WARN("Unable to start transaction: %s", mysql_error(db->handle));
        return DbMapErrorFromConn(db->handle);
    }

    //
    // Execute prepared statements
    //

    result = mysql_stmt_execute(stmtUsers);
    if (result != 0) {
        LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtUsers));
        error = DbMapErrorFromStmt(stmtUsers);
        goto rollback;
    }

    result = mysql_stmt_execute(stmtDelAdmins);
    if (result != 0) {
        LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtDelAdmins));
        error = DbMapErrorFromStmt(stmtDelAdmins);
        goto rollback;
    }

    for (i = 0; i < MAX_GROUPS; i++) {
        groupId = userFile->AdminGroups[i];
        if (groupId == INVALID_GROUP) {
            break;
        }
        if (CopyGroup(groupId, buffer, ELEMENT_COUNT(buffer), &bufferLength)) {
            result = mysql_stmt_execute(stmtAddAdmins);
            if (result != 0) {
                LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtAddAdmins));
                error = DbMapErrorFromStmt(stmtAddAdmins);
                goto rollback;
            }
        }
    }

    result = mysql_stmt_execute(stmtDelGroups);
    if (result != 0) {
        LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtDelGroups));
        error = DbMapErrorFromStmt(stmtDelGroups);
        goto rollback;
    }

    for (i = 0; i < MAX_GROUPS; i++) {
        groupId = userFile->Groups[i];
        if (groupId == INVALID_GROUP) {
            break;
        }
        if (CopyGroup(groupId, buffer, ELEMENT_COUNT(buffer), &bufferLength)) {
            result = mysql_stmt_execute(stmtAddGroups);
            if (result != 0) {
                LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtAddGroups));
                error = DbMapErrorFromStmt(stmtAddGroups);
                goto rollback;
            }
        }
    }

    result = mysql_stmt_execute(stmtDelHosts);
    if (result != 0) {
        LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtDelHosts));
        error = DbMapErrorFromStmt(stmtDelHosts);
        goto rollback;
    }

    for (i = 0; i < MAX_IPS; i++) {
        host = userFile->Ip[i];
        if (host[0] == '\0') {
            break;
        }
        CopyHost(host, buffer, ELEMENT_COUNT(buffer), &bufferLength);

        result = mysql_stmt_execute(stmtAddHosts);
        if (result != 0) {
            LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmtAddHosts));
            error = DbMapErrorFromStmt(stmtAddHosts);
            goto rollback;
        }
    }

    //
    // Commit transaction
    //

    result = mysql_query(db->handle, "COMMIT");
    if (result != 0) {
        LOG_WARN("Unable to commit transaction: %s", mysql_error(db->handle));
        return DbMapErrorFromConn(db->handle);
    }

    return ERROR_SUCCESS;

rollback:
    //
    // Rollback transaction on error
    //

    if (mysql_query(db->handle, "ROLLBACK") != 0) {
        LOG_WARN("Unable to rollback transaction: %s", mysql_error(db->handle));
    }

    ASSERT(error != ERROR_SUCCESS);
    return error;
}

DWORD DbUserClose(USERFILE *userFile)
{
    UNREFERENCED_PARAMETER(userFile);
    return ERROR_SUCCESS;
}
