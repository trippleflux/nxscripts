/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    Database Connection

Author:
    neoxed (neoxed@gmail.com) Jun 13, 2006

Abstract:
    Database connection and management declarations.

*/

#ifndef DB_H_INCLUDED
#define DB_H_INCLUDED

//
// Database structures
//

typedef enum {
    // Table: io_groups
    DB_STMT_GROUP_DELETE,
    DB_STMT_GROUP_CREATE,
    DB_STMT_GROUP_READ,
    DB_STMT_GROUP_EXISTS,
    DB_STMT_GROUP_REFRESH,
    DB_STMT_GROUP_WRITE,

    // Table: io_group_locks
    DB_STMT_GLOCK_DELETE,
    DB_STMT_GLOCK_CREATE,

    // Table: io_users
    DB_STMT_USER_DELETE,
    DB_STMT_USER_CREATE,
    DB_STMT_USER_READ,
    DB_STMT_USER_EXISTS,
    DB_STMT_USER_REFRESH,
    DB_STMT_USER_WRITE,

    // Table: io_user_admins
    DB_STMT_UADMIN_DELETE,
    DB_STMT_UADMIN_CREATE,
    DB_STMT_UADMIN_LIST,

    // Table: io_user_groups
    DB_STMT_UGROUP_DELETE,
    DB_STMT_UGROUP_CREATE,
    DB_STMT_UGROUP_LIST,

    // Table: io_user_hosts
    DB_STMT_UHOST_DELETE,
    DB_STMT_UHOST_CREATE,
    DB_STMT_UHOST_LIST,

    // Table: io_user_locks
    DB_STMT_ULOCK_DELETE,
    DB_STMT_ULOCK_CREATE,

    // End marker
    DB_STMT_END
} DB_STMTS;

typedef struct {
    MYSQL      *handle;             // MySQL connection handle
    MYSQL_STMT *stmt[DB_STMT_END];  // Pre-compiled SQL statements
    UINT64      created;            // Time this context was created
    UINT64      used;               // Time this context was last used
} DB_CONTEXT;

//
// Database functions
//

BOOL DbInit(Io_GetProc *getProc);
VOID DbFinalize(VOID);

BOOL DbAcquire(DB_CONTEXT **dbContext);
VOID DbRelease(DB_CONTEXT *dbContext);

#endif // DB_H_INCLUDED
