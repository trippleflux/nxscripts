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
// Database structures
//

typedef struct {
    MYSQL      *handle;     // MySQL connection handle
    MYSQL_STMT *stmt[8];    // Pre-compiled SQL statements (eighth statement for refresh)
    UINT64      created;    // Time this context was created
    UINT64      used;       // Time this context was last used
} DB_CONTEXT;

typedef struct {
    INT     expire;         // Seconds until a lock expires
    INT     timeout;        // Seconds to wait for a lock to become available
    CHAR    owner[64];      // Lock owner UUID
    SIZE_T  ownerLength;    // Length of the owner UUID
} DB_CONFIG_LOCK;

//
// Database macros
//

#ifdef DEBUG

#define DB_CHECK_PARAMS(binds, stmt)                                            \
{                                                                               \
    ASSERT(ELEMENT_COUNT(binds) == mysql_stmt_param_count(stmt));               \
}

#define DB_CHECK_RESULTS(binds, metadata)                                       \
{                                                                               \
    ASSERT(ELEMENT_COUNT(binds) == mysql_num_fields(metadata));                 \
}

#else // DEBUG

#define DB_CHECK_PARAMS(binds, stmt)

#define DB_CHECK_RESULTS(binds, metadata)

#endif // DEBUG

//
// Database globals
//

extern DB_CONFIG_LOCK dbConfigLock;

//
// Database functions
//

BOOL FCALL DbInit(Io_GetProc *getProc);
VOID FCALL DbFinalize(VOID);

BOOL FCALL DbAcquire(DB_CONTEXT **dbPtr);
VOID FCALL DbRelease(DB_CONTEXT *db);

DWORD FCALL DbMapError(UINT error);

INLINE DWORD FCALL DbMapErrorFromConn(MYSQL *mysql)
{
    ASSERT(mysql != NULL);
    return DbMapError(mysql_errno(mysql));
}

INLINE DWORD FCALL DbMapErrorFromStmt(MYSQL_STMT *stmt)
{
    ASSERT(stmt != NULL);
    return DbMapError(mysql_stmt_errno(stmt));
}

#endif // DATABASE_H_INCLUDED
