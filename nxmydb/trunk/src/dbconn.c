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

static void FreeValues(void);
#if 0
static PoolConstructorProc OpenConn;
static PoolDestructorProc  CloseConn;
#endif

// MySQL server information
static char *serverHost = NULL;
static char *serverUser = NULL;
static char *serverPass = NULL;
static char *serverDb   = NULL;
static int   serverPort = 3306;
static BOOL  useCompression = FALSE;
static BOOL  useEncryption  = FALSE;

#if 0
// Database connection pool
static POOL *pool = NULL;
#endif

// Reference count initialization calls
static int refCount = 0;


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
    int poolMax;
    int poolKeepAlive;
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

    poolMax = poolMin * 2;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Maximum", &poolMax) && poolMax <= poolMin) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Pool_Maximum' must be greater than 'Pool_Minimum'.\r\n");
        return FALSE;
    }

    poolKeepAlive = 3600;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Keep_Alive", &poolKeepAlive) && poolKeepAlive <= 0) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Pool_Keep_Alive' must be greater than zero.\r\n");
        return FALSE;
    }

    poolTimeout = 5;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Timeout", &poolTimeout) && poolTimeout <= 0) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Pool_Timeout' must be greater than zero.\r\n");
        return FALSE;
    }

    // Read server options
    serverHost = Io_ConfigGet("nxMyDB", "Host", NULL, NULL);
    serverUser = Io_ConfigGet("nxMyDB", "User", NULL, NULL);
    serverPass = Io_ConfigGet("nxMyDB", "Password", NULL, NULL);
    serverDb   = Io_ConfigGet("nxMyDB", "Database", NULL, NULL);
    Io_ConfigGetInt("nxMyDB", "Port", &serverPort);
    Io_ConfigGetBool("nxMyDB", "Compression", &useCompression);
    Io_ConfigGetBool("nxMyDB", "Encryption", &useEncryption);

    // Dump configuration
    DebugPrint("Configuration", "   ServerHost=%s\n", serverHost);
    DebugPrint("Configuration", "   ServerPort=%i\n", serverPort);
    DebugPrint("Configuration", "   ServerUser=%s\n", serverUser);
    DebugPrint("Configuration", "   ServerPass=%s\n", serverPass);
    DebugPrint("Configuration", "     ServerDb=%s\n", serverDb);
    DebugPrint("Configuration", "  Compression=%s\n", useCompression ? "true" : "false");
    DebugPrint("Configuration", "   Encryption=%s\n", useEncryption ? "true" : "false");
    DebugPrint("Configuration", "  PoolMinimum=%i\n", poolMin);
    DebugPrint("Configuration", "  PoolMaximum=%i\n", poolMax);
    DebugPrint("Configuration", "PoolKeepAlive=%i\n", poolKeepAlive);
    DebugPrint("Configuration", "  PoolTimeout=%i\n", poolTimeout);

    // Create connection pool
#if 0
    pool = Io_Allocate(sizeof(POOL));
    if (pool == NULL) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Unable to allocate memory for the connection pool.\r\n");
        goto error;
    }
    if (!PoolInit(&pool, poolMin, poolMax, poolKeepAlive, poolTimeout, OpenConn, CloseConn)) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Unable to create connection pool.\r\n");
        goto error;
    }
#endif

    Io_Putlog(LOG_ERROR, "nxMyDB: v%s loaded.\r\n", STRINGIFY(VERSION));
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
#if 0
    if (pool != NULL) {
        PoolDestroy(pool);
        pool = NULL;
    }
#endif
}
