/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2009 neoxed

Module Name:
    Configuration Reader

Author:
    neoxed (neoxed@gmail.com) Jun 30, 2009

Abstract:
    Configuration reading declarations.

*/

#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

//
// Configuration structures
//
typedef struct {
    INT     logLevel;       // Level of log verbosity
} DB_CONFIG_GLOBAL;

typedef struct {
    INT     expire;         // Seconds until a lock expires
    INT     timeout;        // Seconds to wait for a lock to become available
    CHAR    owner[64];      // Lock owner UUID
    SIZE_T  ownerLength;    // Length of the owner UUID
} DB_CONFIG_LOCK;

typedef struct {
    INT     minimum;        // Minimum number of sustained connections
    INT     average;        // Average number of sustained connections
    INT     maximum;        // Maximum number of sustained connections
    INT     timeout;        // Seconds to wait for a connection to become available
    DWORD   timeoutMili;    // Same amount, but in milliseconds

    INT     check;          // Seconds until an idle connection is checked
    UINT64  checkNano;      // Same amount, but in 100 nanosecond intervals
    INT     expire;         // Seconds until a connection expires
    UINT64  expireNano;     // Same amount, but in 100 nanosecond intervals
} DB_CONFIG_POOL;

typedef struct {
    CHAR    *host;          // MySQL Server host
    CHAR    *user;          // MySQL Server username
    CHAR    *password;      // MySQL Server password
    CHAR    *database;      // Database name
    INT      port;          // MySQL Server port
    BOOL     compression;   // Use compression for the server connection
    BOOL     sslEnable;     // Use SSL encryption for the server connection
    CHAR    *sslCiphers;    // List of allowable ciphers to use with SSL encryption
    CHAR    *sslCertFile;   // Path to the certificate file
    CHAR    *sslKeyFile;    // Path to the key file
    CHAR    *sslCAFile;     // Path to the certificate authority file
    CHAR    *sslCAPath;     // Path to the directory containing CA certificates
} DB_CONFIG_SERVER;

typedef struct {
    BOOL         enabled;   // Allow synchronization
    INT          first;     // Milliseconds until the first synchronization
    INT          interval;  // Milliseconds between each database refresh
    INT          purge;     // Seconds to purge old changes entries
} DB_CONFIG_SYNC;

//
// Configuration globals
//

extern DB_CONFIG_GLOBAL  dbConfigGlobal;
extern DB_CONFIG_LOCK    dbConfigLock;
extern DB_CONFIG_POOL    dbConfigPool;
extern DB_CONFIG_SERVER  dbConfigServer;
extern DB_CONFIG_SYNC    dbConfigSync;

//
// Configuration functions
//

DWORD FCALL ConfigInit(VOID);
DWORD FCALL ConfigLoad(VOID);
DWORD FCALL ConfigSetUuid(VOID);
DWORD FCALL ConfigFinalize(VOID);

#endif // CONFIG_H_INCLUDED
