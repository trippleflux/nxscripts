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
    UINT64 created; // Time this context was created
    UINT64 used;    // Time this context was last used
    MYSQL *handle;  // MySQL connection handle
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

#endif // _DB_H_
