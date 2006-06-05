/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Backends

Author:
    neoxed (neoxed@gmail.com) Jun 5, 2006

Abstract:
    User and group storage backends.

*/

#ifndef _BACKENDS_H_
#define _BACKENDS_H_

// Internal context for user/group files.
typedef struct {
    HANDLE fileHandle;
} INT_CONTEXT;


//
// Group database backend
//

BOOL
DbGroupCreate(
    char *groupName,
    INT32 groupId,
    GROUPFILE *groupFile
    );

BOOL
DbGroupRename(
    char *groupName,
    INT32 groupId,
    char *newName
    );

BOOL
DbGroupDelete(
    char *groupName,
    INT32 groupId
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
    INT_CONTEXT *context
    );


//
// Group file backend
//

BOOL
FileGroupCreate(
    char *groupName,
    INT32 groupId,
    GROUPFILE *groupFile
    );

BOOL
FileGroupRename(
    char *groupName,
    INT32 groupId,
    char *newName
    );

BOOL
FileGroupDelete(
    char *groupName,
    INT32 groupId
    );

BOOL
FileGroupLock(
    GROUPFILE *groupFile
    );

BOOL
FileGroupUnlock(
    GROUPFILE *groupFile
    );

BOOL
FileGroupOpen(
    char *groupName,
    GROUPFILE *groupFile
    );

BOOL
FileGroupWrite(
    GROUPFILE *groupFile
    );

BOOL
FileGroupClose(
    INT_CONTEXT *context
    );


//
// User database backend
//

BOOL
DbUserCreate(
    char *userName,
    INT32 userId,
    USERFILE *userFile
    );

BOOL
DbUserRename(
    char *userName,
    INT32 userId,
    char *newName
    );

BOOL
DbUserDelete(
    char *userName,
    INT32 userId
    );

BOOL
DbUserLock(
    USERFILE *userFile
    );

BOOL
DbUserUnlock(
    USERFILE *userFile
    );

BOOL
DbUserOpen(
    char *userName,
    USERFILE *userFile
    );

BOOL
DbUserWrite(
    USERFILE *userFile
    );

BOOL
DbUserClose(
    INT_CONTEXT *context
    );


//
// User file backend
//

BOOL
FileUserCreate(
    char *userName,
    INT32 userId,
    USERFILE *userFile
    );

BOOL
FileUserRename(
    char *userName,
    INT32 userId,
    char *newName
    );

BOOL
FileUserDelete(
    char *userName,
    INT32 userId
    );

BOOL
FileUserLock(
    USERFILE *userFile
    );

BOOL
FileUserUnlock(
    USERFILE *userFile
    );

BOOL
FileUserOpen(
    char *userName,
    USERFILE *userFile
    );

BOOL
FileUserWrite(
    USERFILE *userFile
    );

BOOL
FileUserClose(
    INT_CONTEXT *context
    );

#endif // _BACKENDS_H_
