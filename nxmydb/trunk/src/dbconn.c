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
static int refresh  = 0;
static TIMER *timer = NULL;

// Database connection pool
static POOL *pool = NULL;

// Reference count initialization calls
static int refCount = 0;


/*++

ConnectionOpen

    Opens a database connection.

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
    DB_CONTEXT *context;
    unsigned long flags;

    ASSERT(opaque == NULL);
    ASSERT(data != NULL);
    DebugPrint("ConnectionOpen", "opaque=%p data=%p\n", opaque, data);

    // Allocate database context
    context = Io_Allocate(sizeof(DB_CONTEXT));
    if (context == NULL) {
        DebugPrint("ConnectionOpen", "Unable to allocate database context.\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    // Have MySQL allocate the structure. This is in case the client library is a different
    // version than the header we're compiling with (structures could be different sizes).
    context->handle = mysql_init(NULL);
    if (context->handle == NULL) {
        DebugPrint("ConnectionOpen", "Unable to allocate MySQL handle structure.\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    // Set client options
    flags = CLIENT_INTERACTIVE;
    if (compression) {
        flags |= CLIENT_COMPRESS;
    }
    if (sslEnable) {
        flags |= CLIENT_SSL; // Is this still needed?
        mysql_ssl_set(context->handle, sslKeyFile, sslCertFile, sslCAFile, sslCAPath, sslCiphers);
    }

    // Open database server connection
    if (!mysql_real_connect(context->handle, serverHost, serverUser, serverPass, serverDb, serverPort, NULL, flags)) {
        DebugPrint("ConnectionOpen", "Unable to connect to server: %s\n", mysql_error(context->handle));
        Io_Putlog(LOG_ERROR, "nxMyDB: Unable to connect to server: %s\r\n", mysql_error(context->handle));
        mysql_close(context->handle);

        SetLastError(ERROR_CONNECTION_REFUSED);
        return FALSE;
    }

    // Update access time
    GetSystemTimeAsFileTime((FILETIME *)&context->time);

    DebugPrint("ConnectionOpen", "Connected to %s, running MySQL Server v%s.\n",
        mysql_get_host_info(context->handle), mysql_get_server_info(context->handle));

    *data = context;
    return TRUE;
}

/*++

ConnectionValidate

    Validates the database connection.

Arguments:
    opaque - Opaque argument passed to PoolInit().

    data   - Pointer to a MYSQL structure.

Return Values:
    If the resource is valid, the return is nonzero (true).

    If the resource is invalid, the return is zero (false).

--*/
static
BOOL
ConnectionValidate(
    void *opaque,
    void *data
    )
{
    ASSERT(opaque == NULL);
    ASSERT(data != NULL);
    DebugPrint("ConnectionValidate", "opaque=%p data=%p\n", opaque, data);

    return TRUE;
}

/*++

ConnectionClose

    Closes the database connection.

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
    DB_CONTEXT *context;

    ASSERT(opaque == NULL);
    ASSERT(data != NULL);
    DebugPrint("ConnectionClose", "opaque=%p data=%p\n", opaque, data);

    // Close server connection and free context
    context = data;
    mysql_close(context->handle);
    Io_Free(context);
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
    Number of milliseconds to execute this timer again.

--*/
static
DWORD
RefreshTimer(
    void *timerContext,
    TIMER *timerHandle
    )
{
    DB_CONTEXT *context;
    DebugPrint("RefreshTimer", "timerContext=%d timerHandle=%d\n", timerContext, timerHandle);

    if (DbAcquire(&context)) {
        // Users rely on groups, so update groups first.
        DbGroupRefresh(context);
        DbUserRefresh(context);
    } else {
        DebugPrint("RefreshTimer", "Unable to acquire a database connection.\n");
    }

    // Execute the timer again
    return refresh;
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
    int poolTimeout;
    int poolExpiration;
    int poolValidate;
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

    poolTimeout = 5;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Timeout", &poolTimeout) && poolTimeout <= 0) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Pool_Timeout' must be greater than zero.\r\n");
        return FALSE;
    }
    poolTimeout *= 1000;

    poolExpiration = 3600;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Expiration", &poolExpiration) && poolExpiration <= 0) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Pool_Expiration' must be greater than zero.\r\n");
        return FALSE;
    }
    poolExpiration *= 1000;

    poolValidate = 60;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Validate", &poolValidate) && (poolValidate <= 0 || poolValidate >= poolExpiration)) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Pool_Validate' must be greater than zero and less than 'Pool_Expiration'.\r\n");
        return FALSE;
    }
    poolValidate *= 1000;

    // Refesh timer
    if (Io_ConfigGetInt("nxMyDB", "Refresh", &refresh) && refresh < 0) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Refresh' must be greater than or equal to zero.\r\n");
        return FALSE;
    }
    refresh *= 1000;

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
    DebugPrint("Configuration", "Server Refresh  = %i\n", refresh);
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
    DebugPrint("Configuration", "Pool Timeout    = %i\n", poolTimeout);
    DebugPrint("Configuration", "Pool Expiration = %i\n", poolExpiration);
    DebugPrint("Configuration", "Pool Validate   = %i\n", poolValidate);

    // Create connection pool
    pool = Io_Allocate(sizeof(POOL));
    if (pool == NULL) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Unable to allocate memory for connection pool.\r\n");
        goto error;
    }
    if (!PoolInit(pool, poolMin, poolAvg, poolMax, poolTimeout, poolExpiration, poolValidate,
            ConnectionOpen, ConnectionValidate, ConnectionClose, NULL)) {
        Io_Free(pool);
        Io_Putlog(LOG_ERROR, "nxMyDB: Unable to create connection pool.\r\n");
        goto error;
    }

    // Start database refresh timer
    if (refresh > 0) {
        timer = Io_StartIoTimer(NULL, RefreshTimer, NULL, refresh);
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

    Acquires a database context from the connection pool.

Arguments:
    dbContext - Pointer to a pointer that receives the DB_CONTEXT structure.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
BOOL
DbAcquire(
    DB_CONTEXT **dbContext
    )
{
    DB_CONTEXT *context;
    UINT64 age;

    ASSERT(dbContext != NULL);
    DebugPrint("DbAcquire", "dbContext=%p\n", dbContext);

    // Acquire a database context
    if (!PoolAcquire(pool, &context)) {
        DebugPrint("DbAcquire", "Unable to acquire a database check (error %lu).\n", GetLastError());
        return FALSE;
    }

    // Ping handle if it hasn't been used in more than 60 seconds. Do not
    // convert into seconds before comparing, since that would loose precision.
    GetSystemTimeAsFileTime((FILETIME *)&age);
    age -= context->time;
    if (age > 60 * 10000000) {
        DebugPrint("DbAcquire", "Connection has not been used in %I64u seconds, pinging it.\n", age/10000000);

        if (mysql_ping(context->handle) != 0) {
            DebugPrint("DbAcquire", "Lost server connection: %s\n", mysql_error(context->handle));
            PoolInvalidate(pool, context);

            SetLastError(ERROR_NOT_CONNECTED);
            return FALSE;
        }
    }

    *dbContext = context;
    return TRUE;
}

/*++

DbRelease

    Releases a database context back into the connection pool.

Arguments:
    dbContext - Pointer to a pointer that receives the DB_CONTEXT structure.

Return Values:
    None.

--*/
void
DbRelease(
    DB_CONTEXT *dbContext
    )
{
    ASSERT(dbContext != NULL);
    DebugPrint("DbRelease", "dbContext=%p\n", dbContext);

    // Update access time
    GetSystemTimeAsFileTime((FILETIME *)&dbContext->time);

    // Release the database context
    if (!PoolRelease(pool, dbContext)) {
        DebugPrint("DbRelease", "Unable to release the database context (error %lu).\n", GetLastError());
    }
}
