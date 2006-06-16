/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Database Connection

Author:
    neoxed (neoxed@gmail.com) Jun 13, 2006

Abstract:
    Database connection and initialization declarations.

*/

#ifndef _DBCONN_H_
#define _DBCONN_H_

//
// Database structures
//

typedef struct {
    MYSQL *handle;  // MySQL connection handle
    UINT64 time;    // Time the handle was last used
} DB_CONTEXT;

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

#endif // _DBCONN_H_
