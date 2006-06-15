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
    INT_CONTEXT *context
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
    INT_CONTEXT *context
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
    USERFILE *userFile
    );

BOOL
DbUserRename(
    char *userName,
    char *newName
    );

BOOL
DbUserDelete(
    char *userName
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
    INT_CONTEXT *context
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