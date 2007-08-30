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
// Database structure
//

typedef struct {
    MYSQL      *handle;     // MySQL connection handle
    MYSQL_STMT *stmt;       // Pre-compiled SQL statement
    UINT64      created;    // Time this context was created
    UINT64      used;       // Time this context was last used
} DB_CONTEXT;

//
// Database functions
//

BOOL DbInit(Io_GetProc *getProc);
VOID DbFinalize(VOID);

VOID DbGetConfig(INT *expire, INT *timeout);

BOOL DbAcquire(DB_CONTEXT **dbContext);
VOID DbRelease(DB_CONTEXT *dbContext);

#endif // DB_H_INCLUDED
