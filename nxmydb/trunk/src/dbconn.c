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

// MySQL server information
static char *serverHost = NULL;
static char *serverUser = NULL;
static char *serverPass = NULL;
static char *serverDb   = NULL;
static int   serverPort = 3306;
static BOOL  useCompression = FALSE;
static BOOL  useEncryption  = FALSE;

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

    DebugPrint("DbInit", "Host           =%s\n", serverHost);
    DebugPrint("DbInit", "Port           =%i\n", serverPort);
    DebugPrint("DbInit", "User           =%s\n", serverUser);
    DebugPrint("DbInit", "Password       =%s\n", serverPass);
    DebugPrint("DbInit", "Database       =%s\n", serverDb);
    DebugPrint("DbInit", "Compression    =%s\n", useCompression);
    DebugPrint("DbInit", "Encryption     =%s\n", useEncryption);
    DebugPrint("DbInit", "Pool_Minimum   =%i\n", poolMin);
    DebugPrint("DbInit", "Pool_Maximum   =%i\n", poolMax);
    DebugPrint("DbInit", "Pool_Keep_Alive=%i\n", poolKeepAlive);
    DebugPrint("DbInit", "Pool_Timeout   =%i\n", poolTimeout);

    Io_Putlog(LOG_ERROR, "nxMyDB: v%s loaded.\r\n", STRINGIFY(VERSION));
    return TRUE;
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

        // Free options
        if (serverHost != NULL) {
            Io_Free(serverHost);
        }
        if (serverUser != NULL) {
            Io_Free(serverUser);
        }
        if (serverPass != NULL) {
            Io_Free(serverPass);
        }
        if (serverDb != NULL) {
            Io_Free(serverDb);
        }

        // Clear procedure table
        ProcTableFinalize();
    }
}
