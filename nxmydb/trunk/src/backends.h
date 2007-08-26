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

typedef struct {
    HANDLE fileHandle; // Handle to the open user/group file
} FILE_CONTEXT;


//
// Group database backend
//

BOOL
DbGroupRefresh(
    DB_CONTEXT *dbContext
    );

BOOL
DbGroupCreate(
    DB_CONTEXT *dbContext,
    char *groupName,
    GROUPFILE *groupFile
    );

BOOL
DbGroupRename(
    DB_CONTEXT *dbContext,
    char *groupName,
    char *newName
    );

BOOL
DbGroupDelete(
    DB_CONTEXT *dbContext,
    char *groupName
    );

BOOL
DbGroupLock(
    DB_CONTEXT *dbContext,
    GROUPFILE *groupFile
    );

BOOL
DbGroupUnlock(
    DB_CONTEXT *dbContext,
    GROUPFILE *groupFile
    );

BOOL
DbGroupOpen(
    DB_CONTEXT *dbContext,
    char *groupName,
    GROUPFILE *groupFile
    );

BOOL
DbGroupWrite(
    DB_CONTEXT *dbContext,
    GROUPFILE *groupFile
    );

BOOL
DbGroupClose(
    DB_CONTEXT *dbContext,
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
    FILE_CONTEXT *fileContext
    );

BOOL
FileGroupWrite(
    GROUPFILE *groupFile
    );

BOOL
FileGroupClose(
    FILE_CONTEXT *fileContext
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
    DB_CONTEXT *dbContext,
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
    FILE_CONTEXT *fileContext
    );

BOOL
FileUserWrite(
    USERFILE *userFile
    );

BOOL
FileUserClose(
    FILE_CONTEXT *fileContext
    );

#endif // _BACKENDS_H_
