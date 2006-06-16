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
static char *serverHost  = NULL;
static char *serverUser  = NULL;
static char *serverPass  = NULL;
static char *serverDb    = NULL;
static int   serverPort  = 0;
static BOOL  compression = FALSE;
static BOOL  sslEnable   = FALSE;
static char *sslCiphers  = NULL;
static char *sslCertFile = NULL;
static char *sslKeyFile  = NULL;
static char *sslCAFile   = NULL;
static char *sslCAPath   = NULL;

// Refresh timer
static int refresh = 0;
static TIMER *timer = NULL;

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
    MYSQL *handle;
    unsigned long flags;

    ASSERT(opaque == NULL);
    ASSERT(data != NULL);
    DebugPrint("ConnectionOpen", "opaque=%p data=%p\n", opaque, data);

    // Have MySQL allocate the structure. This is in case the client library is a different
    // version than the header we're compiling with (structures could be different sizes).
    handle = mysql_init(NULL);
    if (handle == NULL) {
        DebugPrint("ConnectionOpen", "Unable to allocate MySQL handle structure.\n");
        return FALSE;
    }

    // Set client options
    flags = CLIENT_INTERACTIVE;
    if (compression) {
        flags |= CLIENT_COMPRESS;
    }
    if (sslEnable) {
        flags |= CLIENT_SSL; // Is this still needed?
        mysql_ssl_set(handle, sslKeyFile, sslCertFile, sslCAFile, sslCAPath, sslCiphers);
    }

    // Open database server connection
    if (!mysql_real_connect(handle, serverHost, serverUser, serverPass, serverDb, serverPort, NULL, flags)) {
        DebugPrint("ConnectionOpen", "Unable to connect to server: %s\r\n", mysql_error(handle));
        Io_Putlog(LOG_ERROR, "nxMyDB: Unable to connect to server: %s\r\n", mysql_error(handle));
        mysql_close(handle);
        return FALSE;
    }

    DebugPrint("ConnectionOpen", "Connected to %s, running MySQL Server v%s.\n",
        mysql_get_host_info(handle), mysql_get_server_info(handle));

    *data = handle;
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
    MYSQL *handle;

    ASSERT(opaque == NULL);
    ASSERT(data != NULL);
    DebugPrint("ConnectionClose", "opaque=%p data=%p\n", opaque, data);

    // Close database server connection
    handle = data;
    mysql_close(handle);
}

/*++

ConfigGet

    Retrieves configuration options, also removing comments and whitespace.

Arguments:
    array    - Option array name.

    variable - Option variable name.

Return Values:
    If the function succeeds, the return value is a pointer to the configuration value.

    If the function fails, the return value is null.

--*/
static
char *
ConfigGet(
    char *array,
    char *variable
    )
{
    char *p;
    char *value;
    size_t length;

    ASSERT(array != NULL);
    ASSERT(variable != NULL);

    // Retrieve value from ioFTPD
    value = Io_ConfigGet(array, variable, NULL, NULL);
    if (value == NULL) {
        return NULL;
    }

    // Count characters before a "#" or ";"
    p = value;
    while (*p != '\0' && *p != '#' && *p != ';') {
        p++;
    }
    length = p - value;

    // Strip trailing whitespace
    while (length > 0 && isspace(value[length-1])) {
        length--;
    }

    value[length] = '\0';
    return value;
}

/*++

ConfigFree

    Frees memory allocated for configuration options.

Arguments:
    None.

Return Values:
    None.

--*/
static
void
ConfigFree(
    void
    )
{
    DebugPrint("ConfigFree", "refCount=%i\n", refCount);

    // Free server options
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

    // Free SSL options
    if (sslCiphers != NULL) {
        Io_Free(sslCiphers);
    }
    if (sslCertFile != NULL) {
        Io_Free(sslCertFile);
    }
    if (sslKeyFile != NULL) {
        Io_Free(sslKeyFile);
    }
    if (sslCAFile != NULL) {
        Io_Free(sslCAFile);
    }
    if (sslCAPath != NULL) {
        Io_Free(sslCAPath);
    }
}

/*++

RefreshTimer

    Refreshes the local user and group cache.

Arguments:
    None.

Return Values:
    None.

--*/
static
void
RefreshTimer(
    WPARAM foo,
    LPARAM bar
    )
{
    MYSQL *handle;
    DebugPrint("RefreshTimer", "foo=%d bar=%d\n", foo, bar);

    if (!DbAcquire(&handle)) {
        DebugPrint("RefreshTimer", "Unable to acquire a database connection.\n");
        return;
    }

    // Users rely on groups, so update groups first.
    DbGroupRefresh(handle);
    DbUserRefresh(handle);
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
    void *result;
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
    poolMin = 1;
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
    poolExpiration *= 1000;

    poolTimeout = 5;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Timeout", &poolTimeout) && poolTimeout <= 0) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Pool_Timeout' must be greater than zero.\r\n");
        return FALSE;
    }
    poolTimeout *= 1000;

    // Refesh timer
    if (Io_ConfigGetInt("nxMyDB", "Refresh", &refresh) && refresh < 0) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Refresh' must be greater than or equal to zero.\r\n");
        return FALSE;
    }

    // Read server options
    serverHost = ConfigGet("nxMyDB", "Host");
    serverUser = ConfigGet("nxMyDB", "User");
    serverPass = ConfigGet("nxMyDB", "Password");
    serverDb   = ConfigGet("nxMyDB", "Database");
    Io_ConfigGetInt("nxMyDB", "Port", &serverPort);
    Io_ConfigGetBool("nxMyDB", "Compression", &compression);
    Io_ConfigGetBool("nxMyDB", "SSL_Enable", &sslEnable);
    sslCiphers  = ConfigGet("nxMyDB", "SSL_Ciphers");
    sslCertFile = ConfigGet("nxMyDB", "SSL_Cert_File");
    sslKeyFile  = ConfigGet("nxMyDB", "SSL_Key_File");
    sslCAFile   = ConfigGet("nxMyDB", "SSL_CA_File");
    sslCAPath   = ConfigGet("nxMyDB", "SSL_CA_Path");

    // Dump configuration
    DebugPrint("Configuration", "Server Host     = %s\n", serverHost);
    DebugPrint("Configuration", "Server Port     = %i\n", serverPort);
    DebugPrint("Configuration", "Server User     = %s\n", serverUser);
    DebugPrint("Configuration", "Server Password = %s\n", serverPass);
    DebugPrint("Configuration", "Server Database = %s\n", serverDb);
    DebugPrint("Configuration", "Compression     = %s\n", compression ? "true" : "false");
    DebugPrint("Configuration", "SSL Enable      = %s\n", sslEnable ? "true" : "false");
    DebugPrint("Configuration", "SSL Ciphers     = %s\n", sslCiphers);
    DebugPrint("Configuration", "SSL Cert File   = %s\n", sslCertFile);
    DebugPrint("Configuration", "SSL Key File    = %s\n", sslKeyFile);
    DebugPrint("Configuration", "SSL CA File     = %s\n", sslCAFile);
    DebugPrint("Configuration", "SSL CA Path     = %s\n", sslCAPath);
    DebugPrint("Configuration", "Pool Minimum    = %i\n", poolMin);
    DebugPrint("Configuration", "Pool Average    = %i\n", poolAvg);
    DebugPrint("Configuration", "Pool Maximum    = %i\n", poolMax);
    DebugPrint("Configuration", "Pool Expiration = %i\n", poolExpiration);
    DebugPrint("Configuration", "Pool Timeout    = %i\n", poolTimeout);

    result = Io_StartIoTimer(&timer, RefreshTimer, 10, 5000);
    DebugPrint("Timer", "result=%p timer=%p\n", result, timer);

    // Create connection pool
    pool = Io_Allocate(sizeof(POOL));
    if (pool == NULL) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Unable to allocate memory for connection pool.\r\n");
        goto error;
    }
    if (!PoolInit(pool, poolMin, poolAvg, poolMax, poolExpiration,
            poolTimeout, ConnectionOpen, ConnectionClose, NULL)) {
        Io_Free(pool);
        Io_Putlog(LOG_ERROR, "nxMyDB: Unable to create connection pool.\r\n");
        goto error;
    }

    Io_Putlog(LOG_ERROR, "nxMyDB: v%s loaded, using MySQL Client Library v%s.\r\n",
        STRINGIFY(VERSION), mysql_get_client_info());
    return TRUE;

error:
    ConfigFree();
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

        // Destroy connection pool
        PoolDestroy(pool);
        Io_Free(pool);

        // Free configuration values
        ConfigFree();

        // Clear procedure table
        ProcTableFinalize();
    }
}

/*++

DbAcquire

    Acquires a MySQL handle from the connection pool.

Arguments:
    handle  - Pointer to a pointer that receives the MYSQL handle structure.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
BOOL
DbAcquire(
    MYSQL **handle
    )
{
    ASSERT(handle != NULL);
    DebugPrint("DbAcquire", "handle=%p\n", handle);

    return TRUE;
}

/*++

DbRelease

    Releases a MySQL handle back into the connection pool.

Arguments:
    handle  - Pointer to a pointer that receives the MYSQL handle structure.

Return Values:
    None.

--*/
void
DbRelease(
    MYSQL *handle
    )
{
    ASSERT(handle != NULL);
    DebugPrint("DbRelease", "handle=%p\n", handle);
}
