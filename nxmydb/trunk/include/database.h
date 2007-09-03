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

#ifndef DATABASE_H_INCLUDED
#define DATABASE_H_INCLUDED

//
// Database structure
//

typedef struct {
    INT     expire;         // Lock expiration
    INT     timeout;        // Lock timeout
    CHAR    owner[64];      // Lock owner UUID
    SIZE_T  ownerLength;    // Length of owner UUID
} DB_CONFIG_LOCK;

typedef struct {
    MYSQL      *handle;     // MySQL connection handle
    MYSQL_STMT *stmt[7];    // Pre-compiled SQL statements
    UINT64      created;    // Time this context was created
    UINT64      used;       // Time this context was last used
} DB_CONTEXT;

//
// Database macros
//

#ifdef DEBUG

#define DB_CHECK_BINDS(binds, stmt)                                             \
{                                                                               \
    ASSERT(ELEMENT_COUNT(binds) == mysql_stmt_param_count(stmt));               \
}

#else // DEBUG

#define DB_CHECK_BINDS(binds, context)

#endif // DEBUG

//
// Database globals
//

extern DB_CONFIG_LOCK dbConfigLock;

BOOL FCALL DbInit(Io_GetProc *getProc);
VOID FCALL DbFinalize(VOID);

DWORD FCALL DbMapError(INT result);

BOOL FCALL DbAcquire(DB_CONTEXT **dbContext);
VOID FCALL DbRelease(DB_CONTEXT *dbContext);

#endif // DATABASE_H_INCLUDED
