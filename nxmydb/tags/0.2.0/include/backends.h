/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    Backends

Author:
    neoxed (neoxed@gmail.com) Jun 5, 2006

Abstract:
    User and group storage backend declarations.

*/

#include <database.h>

#ifndef BACKENDS_H_INCLUDED
#define BACKENDS_H_INCLUDED

//
// Module definitions
//

#define MODULE_NAME         "NXMYDB"
#define MODULE_NAME_LENGTH  6

typedef struct {
    HANDLE  file;   // Handle to the user/group file
} MOD_CONTEXT;


//
// Group module functions
//

DWORD GroupRegister(CHAR *groupName, GROUPFILE *groupFile, INT32 *groupIdPtr);
DWORD GroupRegisterAs(CHAR *groupName, CHAR *newName);
DWORD GroupUnregister(CHAR *groupName);
DWORD GroupUpdate(GROUPFILE *groupFile);
DWORD GroupUpdateByName(CHAR *groupName, GROUPFILE *groupFile);

//
// Group database backend
//

DWORD DbGroupRead(DB_CONTEXT *db, CHAR *groupName, GROUPFILE *groupFile);

DWORD DbGroupCreate(DB_CONTEXT *dbContext, CHAR *groupName, GROUPFILE *groupFile);
DWORD DbGroupRename(DB_CONTEXT *dbContext, CHAR *groupName, CHAR *newName);
DWORD DbGroupDelete(DB_CONTEXT *dbContext, CHAR *groupName);
DWORD DbGroupLock(DB_CONTEXT *dbContext, CHAR *groupName, GROUPFILE *groupFile);
DWORD DbGroupUnlock(DB_CONTEXT *dbContext, CHAR *groupName);
DWORD DbGroupOpen(DB_CONTEXT *dbContext, CHAR *groupName, GROUPFILE *groupFile);
DWORD DbGroupWrite(DB_CONTEXT *dbContext, CHAR *groupName, GROUPFILE *groupFile);
DWORD DbGroupClose(GROUPFILE *groupFile);

//
// Group file backend
//

DWORD FileGroupDefault(GROUPFILE *groupFile);

DWORD FileGroupCreate(INT32 groupId, GROUPFILE *groupFile);
DWORD FileGroupDelete(INT32 groupId);
DWORD FileGroupOpen(INT32 groupId, GROUPFILE *groupFile);
DWORD FileGroupWrite(GROUPFILE *groupFile);
DWORD FileGroupClose(GROUPFILE *groupFile);


//
// User module functions
//

DWORD UserRegister(CHAR *userName, USERFILE *userFile, INT32 *userIdPtr);
DWORD UserRegisterAs(CHAR *userName, CHAR *newName);
DWORD UserUnregister(CHAR *userName);
DWORD UserUpdate(USERFILE *userFile);
DWORD UserUpdateByName(CHAR *userName, USERFILE *userFile);

//
// User database backend
//

DWORD DbUserRead(DB_CONTEXT *db, CHAR *userName, USERFILE *userFile);
DWORD DbUserReadExtra(DB_CONTEXT *db, CHAR *userName, USERFILE *userFile);

DWORD DbUserCreate(DB_CONTEXT *dbContext, CHAR *userName, USERFILE *userFile);
DWORD DbUserRename(DB_CONTEXT *dbContext, CHAR *userName, CHAR *newName);
DWORD DbUserDelete(DB_CONTEXT *dbContext, CHAR *userName);
DWORD DbUserLock(DB_CONTEXT *dbContext, CHAR *userName, USERFILE *userFile);
DWORD DbUserUnlock(DB_CONTEXT *dbContext, CHAR *userName);
DWORD DbUserOpen(DB_CONTEXT *dbContext, CHAR *userName, USERFILE *userFile);
DWORD DbUserWrite(DB_CONTEXT *dbContext, CHAR *userName, USERFILE *userFile);
DWORD DbUserClose(USERFILE *userFile);

//
// User file backend
//

DWORD FileUserDefault(USERFILE *userFile);

DWORD FileUserCreate(INT32 userId, USERFILE *userFile);
DWORD FileUserDelete(INT32 userId);
DWORD FileUserOpen(INT32 userId, USERFILE *userFile);
DWORD FileUserWrite(USERFILE *userFile);
DWORD FileUserClose(USERFILE *userFile);


//
// Database syncronization
//

typedef enum {
    SYNC_EVENT_CREATE = 0,
    SYNC_EVENT_RENAME = 1,
    SYNC_EVENT_DELETE = 2,
} SYNC_EVENT;

typedef struct {
    VOID    *foobar;
} SYNC_LOCAL;

typedef struct {
    ULONG       currUpdate; // Server time for the current update
    ULONG       prevUpdate; // Server time of the last update
    SYNC_LOCAL  group;      // Group syncronization context
    SYNC_LOCAL  user;       // User syncronization context
} SYNC_CONTEXT;

DWORD DbGroupSync(DB_CONTEXT *db, SYNC_CONTEXT *sync);
DWORD DbUserSync(DB_CONTEXT *db, SYNC_CONTEXT *sync);

#endif // BACKENDS_H_INCLUDED
