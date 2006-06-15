/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Database Connection

Author:
    neoxed (neoxed@gmail.com) Jun 13, 2006

Abstract:
    Database connection and initialization functions.

*/

#include "mydb.h"

// MySQL Server information
static char *serverHost = NULL;
static char *serverUser = NULL;
static char *serverPass = NULL;
static char *serverDb   = NULL;
static int   serverPort = 3306;
static BOOL  useCompression = FALSE;
static BOOL  useEncryption  = FALSE;

// Database connection pool
static POOL *pool = NULL;

// Reference count initialization calls
static int refCount = 0;


/*++

ConnectionOpen

    Opens a MySQL Server connection.

Arguments:
    opaque - Opaque argument passed to PoolInit().

    data   - Pointer to a MYSQL structure.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
static
BOOL
ConnectionOpen(
    void *opaque,
    void **data
    )
{
    ASSERT(opaque == NULL);
    ASSERT(data != NULL);
    DebugPrint("ConnectionOpen", "opaque=%p data=%p\n", opaque, data);

    return TRUE;
}

/*++

ConnectionClose

    Closes the MySQL Server connection.

Arguments:
    opaque - Opaque argument passed to PoolInit().

    data   - Pointer to a MYSQL structure.

Return Values:
    None.

--*/
static
void
ConnectionClose(
    void *opaque,
    void *data
    )
{
    ASSERT(opaque == NULL);
    ASSERT(data != NULL);
    DebugPrint("ConnectionClose", "opaque=%p data=%p\n", opaque, data);
}

/*++

FreeValues

    Frees memory allocated for configuration options and connection pools.

Arguments:
    None.

Return Values:
    None.

--*/
static
void
FreeValues(
    void
    )
{
    DebugPrint("FreeValues", "refCount=%i\n", refCount);

    if (serverHost != NULL) {
        Io_Free(serverHost);
        serverHost = NULL;
    }
    if (serverUser != NULL) {
        Io_Free(serverUser);
        serverUser = NULL;
    }
    if (serverPass != NULL) {
        Io_Free(serverPass);
        serverPass = NULL;
    }
    if (serverDb != NULL) {
        Io_Free(serverDb);
        serverDb = NULL;
    }
    if (pool != NULL) {
        PoolDestroy(pool);
        pool = NULL;
    }
}


/*++

DbInit

    Initializes the procedure table and database connection pool.

Arguments:
    getProc - Pointer to ioFTPD's GetProc function.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

Remarks:
    This function must be called once by each module entry point. Synchronization is not
    important at this point because ioFTPD performs all module loading and initialization
    in a single thread at start-up.

--*/
BOOL
DbInit(
    Io_GetProc *getProc
    )
{
    int poolMin;
    int poolAvg;
    int poolMax;
    int poolExpiration;
    int poolTimeout;
    DebugPrint("DbInit", "getProc=%p refCount=%i\n", getProc, refCount);

    // Only initialize the module once
    if (refCount++) {
        DebugPrint("DbInit", "Already initialized, returning.\n");
        return TRUE;
    }

    // Initialize procedure table
    if (!ProcTableInit(getProc)) {
        DebugPrint("DbInit", "Unable to initialize procedure table.\n");
        return FALSE;
    }

    // Read pool options
    poolMin = 2;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Minimum", &poolMin) && poolMin <= 0) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Pool_Minimum' must be greater than zero.\r\n");
        return FALSE;
    }

    poolAvg = poolMin + 1;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Average", &poolAvg) && poolAvg < poolMin) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Pool_Average' must be greater than or equal to 'Pool_Minimum'.\r\n");
        return FALSE;
    }

    poolMax = poolAvg * 2;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Maximum", &poolMax) && poolMax < poolAvg) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Pool_Maximum' must be greater than or equal to 'Pool_Average'.\r\n");
        return FALSE;
    }

    poolExpiration = 3600;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Expiration", &poolExpiration) && poolExpiration <= 0) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Pool_Expiration' must be greater than zero.\r\n");
        return FALSE;
    }

    poolTimeout = 5;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Timeout", &poolTimeout) && poolTimeout <= 0) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Pool_Timeout' must be greater than zero.\r\n");
        return FALSE;
    }

    // Seconds to milliseconds
    poolExpiration *= 1000;
    poolTimeout *= 1000;

    // Read server options
    serverHost = Io_ConfigGet("nxMyDB", "Host", NULL, NULL);
    serverUser = Io_ConfigGet("nxMyDB", "User", NULL, NULL);
    serverPass = Io_ConfigGet("nxMyDB", "Password", NULL, NULL);
    serverDb   = Io_ConfigGet("nxMyDB", "Database", NULL, NULL);
    Io_ConfigGetInt("nxMyDB", "Port", &serverPort);
    Io_ConfigGetBool("nxMyDB", "Compression", &useCompression);
    Io_ConfigGetBool("nxMyDB", "Encryption", &useEncryption);

    // Dump configuration
    DebugPrint("Configuration", "    ServerHost = %s\n", serverHost);
    DebugPrint("Configuration", "    ServerPort = %i\n", serverPort);
    DebugPrint("Configuration", "    ServerUser = %s\n", serverUser);
    DebugPrint("Configuration", "    ServerPass = %s\n", serverPass);
    DebugPrint("Configuration", "      ServerDb = %s\n", serverDb);
    DebugPrint("Configuration", "   Compression = %s\n", useCompression ? "true" : "false");
    DebugPrint("Configuration", "    Encryption = %s\n", useEncryption ? "true" : "false");
    DebugPrint("Configuration", "   PoolMinimum = %i\n", poolMin);
    DebugPrint("Configuration", "   PoolAverage = %i\n", poolAvg);
    DebugPrint("Configuration", "   PoolMaximum = %i\n", poolMax);
    DebugPrint("Configuration", "PoolExpiration = %i\n", poolExpiration);
    DebugPrint("Configuration", "   PoolTimeout = %i\n", poolTimeout);

    // Create connection pool
    pool = Io_Allocate(sizeof(POOL));
    if (pool == NULL) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Unable to allocate memory for connection pool.\r\n");
        goto error;
    }
    if (!PoolInit(pool, poolMin, poolAvg, poolMax, poolExpiration,
            poolTimeout, ConnectionOpen, ConnectionClose, NULL)) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Unable to create connection pool.\r\n");
        goto error;
    }

    Io_Putlog(LOG_ERROR, "nxMyDB: v%s loaded, using MySQL client v%s.\r\n",
        STRINGIFY(VERSION), mysql_get_client_info());
    return TRUE;

error:
    FreeValues();
    return FALSE;
}

/*++

DbFinalize

    Finalizes the procedure table and database connection pool.

Arguments:
    None.

Return Values:
    None.

Remarks:
    This function must be called once by each module exit point.

--*/
void
DbFinalize(
    void
    )
{
    DebugPrint("DbFinalize", "refCount=%i\n", refCount);

    // Finalize once the reference count reaches zero
    if (--refCount == 0) {
        Io_Putlog(LOG_ERROR, "nxMyDB: v%s unloaded.\r\n", STRINGIFY(VERSION));

        FreeValues();
        ProcTableFinalize();
    }
}

/*++

DbAcquire

    Acquires a MySQL handle from the connection pool.

Arguments:
    mysql   - Pointer to a pointer that receives the MYSQL structure.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
BOOL
DbAcquire(
    MYSQL **mysql
    )
{
    ASSERT(mysql != NULL);
    DebugPrint("DbAcquire", "mysql=%p\n", mysql);

    return TRUE;
}

/*++

DbRelease

    Releases a MySQL handle back into the connection pool.

Arguments:
    mysql  - Pointer to a pointer that receives the MYSQL structure.

Return Values:
    None.

--*/
void
DbRelease(
    MYSQL *mysql
    )
{
    ASSERT(mysql != NULL);
    DebugPrint("DbRelease", "mysql=%p\n", mysql);
}
