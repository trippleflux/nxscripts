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
    UINT64      created;    // Time this context was created
    UINT64      used;       // Time this context was last used
    MYSQL      *handle;     // MySQL connection handle
    MYSQL_STMT *stmt[23];   // Pre-compiled SQL statements
} DB_CONTEXT;

enum {
    // locking
    STMT_LOCK=0,
    STMT_UNLOCK,

    // io_groups
    STMT_GROUPS_DELETE,
    STMT_GROUPS_CREATE,
    STMT_GROUPS_READ,
    STMT_GROUPS_LIST,
    STMT_GROUPS_REFRESH,
    STMT_GROUPS_WRITE,

    // io_users
    STMT_USERS_DELETE,
    STMT_USERS_CREATE,
    STMT_USERS_READ,
    STMT_USERS_LIST,
    STMT_USERS_REFRESH,
    STMT_USERS_WRITE,

    // io_useradmins
    STMT_UADMINS_DELETE,
    STMT_UADMINS_CREATE,
    STMT_UADMINS_LIST,

    // io_usergroups
    STMT_UGROUPS_DELETE,
    STMT_UGROUPS_CREATE,
    STMT_UGROUPS_LIST,

    // io_userhosts
    STMT_UHOSTS_DELETE,
    STMT_UHOSTS_CREATE,
    STMT_UHOSTS_LIST
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
