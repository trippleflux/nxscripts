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
    MYSQL_STMT *stmt[7];    // Pre-compiled SQL statements
    UINT64      created;    // Time this context was created
    UINT64      used;       // Time this context was last used
} DB_CONTEXT;

typedef struct {
    INT     expire;         // Lock expiration
    INT     timeout;        // Lock timeout
    CHAR    owner[64];      // Lock owner UUID
    SIZE_T  ownerLength;    // Length of owner UUID
} DB_CONFIG_LOCK;

typedef struct {
    void *foo;
} DB_CONFIG_POOL;

typedef struct {
    CHAR    *serverHost;    // MySQL Server host
    CHAR    *serverUser;    // MySQL Server username
    CHAR    *serverPass;    // MySQL Server password
    CHAR    *serverDb;      // Database name
    INT      serverPort;    // MySQL Server port
    BOOL     compression;   // Use compression for the server connection
    BOOL     sslEnable;     // Use SSL encryption for the server connection
    CHAR    *sslCiphers;    // List of allowable ciphers to use with SSL encryption
    CHAR    *sslCertFile;   // Path to the certificate file
    CHAR    *sslKeyFile;    // Path to the key file
    CHAR    *sslCAFile;     // Path to the certificate authority file
    CHAR    *sslCAPath;     // Path to the directory containing CA certificates
} DB_CONFIG_SERVER;

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

extern DB_CONFIG_LOCK   dbConfigLock;
extern DB_CONFIG_POOL   dbConfigPool;
extern DB_CONFIG_SERVER dbConfigServer;

BOOL FCALL DbInit(Io_GetProc *getProc);
VOID FCALL DbFinalize(VOID);

DWORD FCALL DbMapError(INT result);

BOOL FCALL DbAcquire(DB_CONTEXT **dbContext);
VOID FCALL DbRelease(DB_CONTEXT *dbContext);

#endif // DATABASE_H_INCLUDED
