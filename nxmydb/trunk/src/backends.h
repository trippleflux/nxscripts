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

#ifndef _USER_H_
#define _USER_H_

typedef struct {
    HANDLE fileHandle;
} INT_CONTEXT;

//
// User database backend
//

#define DbUserCreate(a,b,c) TRUE
#define DbUserRename(a,b,c) TRUE
#define DbUserDelete(a,b)   TRUE
#define DbUserLock(a)       TRUE
#define DbUserUnlock(a)     TRUE
#define DbUserOpen(a,b)     TRUE
#define DbUserWrite(a)      TRUE
#define DbUserClose(a)      TRUE

#if 0
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
#endif

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

#endif // _USER_H_
