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
    MYSQL      *handle;     // MySQL connection handle
    MYSQL_STMT *stmt;       // Pre-compiled SQL statement
    UINT64      created;    // Time this context was created
    UINT64      used;       // Time this context was last used
} DB_CONTEXT;

//
// Database macros
//

#ifdef DEBUG

#define DB_CHECK_BINDS(binds, context)                                          \
{                                                                               \
    ASSERT(ELEMENT_COUNT(binds) == mysql_stmt_param_count(context->stmt));      \
}

#else // DEBUG

#define DB_CHECK_BINDS(binds, context)

#endif // DEBUG

//
// Database functions
//

BOOL FCALL DbInit(Io_GetProc *getProc);
VOID FCALL DbFinalize(VOID);

VOID  FCALL DbGetConfig(INT *expire, INT *timeout, CHAR **owner);
DWORD FCALL DbMapError(INT result);

BOOL FCALL DbAcquire(DB_CONTEXT **dbContext);
VOID FCALL DbRelease(DB_CONTEXT *dbContext);

#endif // DATABASE_H_INCLUDED
