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

enum {
    // Locking
    STMT_LOCK = 0,
    STMT_UNLOCK,

    // Table: io_groups
    STMT_GROUP_DELETE,
    STMT_GROUP_CREATE,
    STMT_GROUP_READ,
    STMT_GROUP_EXISTS,
    STMT_GROUP_REFRESH,
    STMT_GROUP_WRITE,

    // Table: io_users
    STMT_USER_DELETE,
    STMT_USER_CREATE,
    STMT_USER_READ,
    STMT_USER_EXISTS,
    STMT_USER_REFRESH,
    STMT_USER_WRITE,

    // Table: io_useradmins
    STMT_UADMIN_DELETE,
    STMT_UADMIN_CREATE,
    STMT_UADMIN_LIST,

    // Table: io_usergroups
    STMT_UGROUP_DELETE,
    STMT_UGROUP_CREATE,
    STMT_UGROUP_LIST,

    // Table: io_userhosts
    STMT_UHOST_DELETE,
    STMT_UHOST_CREATE,
    STMT_UHOST_LIST
};

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

#endif // _DB_H_
