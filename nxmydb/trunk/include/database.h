/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2009 neoxed

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

typedef union {
    FILETIME    fileTime;   // FILETIME structure
    UINT64      value;      // Unsigned 64bit value
} DB_TIME;

typedef struct {
    MYSQL      *handle;     // MySQL connection handle
    MYSQL_STMT *stmt[8];    // Pre-compiled SQL statements (eighth statement for refresh)
    LONG        index;      // Index in the server configuration array
    DB_TIME     created;    // Time this context was created
    DB_TIME     used;       // Time this context was last used
} DB_CONTEXT;

typedef struct {
    ULONG       currUpdate; // Server time for the current update
    ULONG       prevUpdate; // Server time of the last update
    TIMER       *timer;     // Synchronization timer
} DB_SYNC;

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
// Database functions
//

BOOL FCALL DbInit(Io_GetProc *getProc);
VOID FCALL DbFinalize(VOID);

VOID FCALL DbSyncPurge(VOID);
VOID FCALL DbSyncStart(VOID);
VOID FCALL DbSyncStop(VOID);

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
