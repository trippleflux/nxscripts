/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Backends

Author:
    neoxed (neoxed@gmail.com) Jun 5, 2006

Abstract:
    User and group storage backend declarations.

*/

#ifndef _BACKENDS_H_
#define _BACKENDS_H_

// Context for users and groups
typedef struct {
    HANDLE      fileHandle; // Handle to the open user/group file
    DB_CONTEXT *dbReserved; // Reserved database connection (acquired on lock, released on unlock)
} GROUP_CONTEXT, USER_CONTEXT;


//
// Group database backend
//

BOOL
DbGroupCreate(
    char *groupName,
    GROUPFILE *groupFile
    );

BOOL
DbGroupRename(
    char *groupName,
    char *newName
    );

BOOL
DbGroupDelete(
    char *groupName
    );

BOOL
DbGroupLock(
    GROUPFILE *groupFile
    );

BOOL
DbGroupUnlock(
    GROUPFILE *groupFile
    );

BOOL
DbGroupOpen(
    char *groupName,
    GROUPFILE *groupFile
    );

BOOL
DbGroupWrite(
    GROUPFILE *groupFile
    );

BOOL
DbGroupClose(
    GROUP_CONTEXT *context
    );

BOOL
DbGroupRefresh(
    DB_CONTEXT *context
    );


//
// Group file backend
//

BOOL
FileGroupCreate(
    INT32 groupId,
    GROUPFILE *groupFile
    );

BOOL
FileGroupDelete(
    INT32 groupId
    );

BOOL
FileGroupOpen(
    INT32 groupId,
    GROUP_CONTEXT *context
    );

BOOL
FileGroupWrite(
    GROUPFILE *groupFile
    );

BOOL
FileGroupClose(
    GROUP_CONTEXT *context
    );


//
// User database backend
//

BOOL
DbUserRefresh(
    DB_CONTEXT *dbContext
    );

BOOL
DbUserCreate(
    DB_CONTEXT *dbContext,
    char *userName,
    USERFILE *userFile
    );

BOOL
DbUserRename(
    DB_CONTEXT *dbContext,
    char *userName,
    char *newName
    );

BOOL
DbUserDelete(
    DB_CONTEXT *dbContext,
    char *userName
    );

BOOL
DbUserLock(
    DB_CONTEXT *dbContext,
    USERFILE *userFile
    );

BOOL
DbUserUnlock(
    DB_CONTEXT *dbContext,
    USERFILE *userFile
    );

BOOL
DbUserOpen(
    DB_CONTEXT *dbContext,
    char *userName,
    USERFILE *userFile
    );

BOOL
DbUserWrite(
    DB_CONTEXT *dbContext,
    USERFILE *userFile
    );

BOOL
DbUserClose(
    USER_CONTEXT *userContext
    );


//
// User file backend
//

BOOL
FileUserCreate(
    INT32 userId,
    USERFILE *userFile
    );

BOOL
FileUserDelete(
    INT32 userId
    );

BOOL
FileUserOpen(
    INT32 userId,
    USER_CONTEXT *context
    );

BOOL
FileUserWrite(
    USERFILE *userFile
    );

BOOL
FileUserClose(
    USER_CONTEXT *context
    );

#endif // _BACKENDS_H_
