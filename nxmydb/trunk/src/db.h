/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Database Connection

Author:
    neoxed (neoxed@gmail.com) Jun 13, 2006

Abstract:
    Database connection and management declarations.

*/

#ifndef _DB_H_
#define _DB_H_

//
// Database structures
//

typedef struct {
    MYSQL      *handle;     // MySQL connection handle
    MYSQL_STMT *stmt[23];   // Pre-compiled SQL statements
    UINT64      created;    // Time this context was created
    UINT64      used;       // Time this context was last used
} DB_CONTEXT;

typedef enum {
    // Locking
    DB_STMT_LOCK = 0,
    DB_STMT_UNLOCK,

    // Table: io_groups
    DB_STMT_GROUP_DELETE,
    DB_STMT_GROUP_CREATE,
    DB_STMT_GROUP_READ,
    DB_STMT_GROUP_EXISTS,
    DB_STMT_GROUP_REFRESH,
    DB_STMT_GROUP_WRITE,

    // Table: io_users
    DB_STMT_USER_DELETE,
    DB_STMT_USER_CREATE,
    DB_STMT_USER_READ,
    DB_STMT_USER_EXISTS,
    DB_STMT_USER_REFRESH,
    DB_STMT_USER_WRITE,

    // Table: io_useradmins
    DB_STMT_UADMIN_DELETE,
    DB_STMT_UADMIN_CREATE,
    DB_STMT_UADMIN_LIST,

    // Table: io_usergroups
    DB_STMT_UGROUP_DELETE,
    DB_STMT_UGROUP_CREATE,
    DB_STMT_UGROUP_LIST,

    // Table: io_userhosts
    DB_STMT_UHOST_DELETE,
    DB_STMT_UHOST_CREATE,
    DB_STMT_UHOST_LIST
} DB_STMTS;

typedef enum {
    LOCK_TYPE_USER = 0,
    LOCK_TYPE_GROUP
} DB_LOCK_TYPE;

//
// Database functions
//

BOOL
DbInit(
    Io_GetProc *getProc
    );

void
DbFinalize(
    void
    );

BOOL
DbAcquire(
    DB_CONTEXT **dbContext
    );

void
DbRelease(
    DB_CONTEXT *dbContext
    );

BOOL
DbLock(
    DB_CONTEXT *dbContext,
    DB_LOCK_TYPE lockType,
    const char *lockName
    );

void
DbUnlock(
    DB_CONTEXT *dbContext,
    DB_LOCK_TYPE lockType,
    const char *lockName
    );

#endif // _DB_H_
