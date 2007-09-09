/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    Database Connection

Author:
    neoxed (neoxed@gmail.com) Jun 13, 2006

Abstract:
    Database connection and management functions.

*/

#include <base.h>
#include <backends.h>
#include <database.h>
#include <pool.h>

#include <errmsg.h>
#include <rpc.h>

//
// Configuration variables
//

DB_CONFIG_LOCK    dbConfigLock;
DB_CONFIG_SERVER  dbConfigServer;

static DB_CONFIG_POOL    dbConfigPool;
static DB_CONFIG_REFRESH dbConfigRefresh;

static POOL *pool;          // Database connection pool
static INT  refCount = 0;    // Reference count initialization calls

//
// Function declarations
//

static POOL_CONSTRUCTOR_PROC ConnectionOpen;
static POOL_VALIDATOR_PROC   ConnectionCheck;
static POOL_DESTRUCTOR_PROC  ConnectionClose;

static VOID  FCALL ConfigInit(VOID);
static BOOL  FCALL ConfigLoad(VOID);
static VOID  FCALL ConfigFree(VOID);

static CHAR *FCALL ConfigGet(CHAR *array, CHAR *variable);
static BOOL  FCALL ConfigUuid(VOID);


/*++

ConnectionOpen

    Opens a server connection.

Arguments:
    opaque  - Opaque argument passed to PoolCreate().

    data    - Pointer to a pointer that receives the DB_CONTEXT structure.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
static BOOL FCALL ConnectionOpen(VOID *opaque, VOID **data)
{
    DB_CONTEXT      *context;
    DWORD           error;
    DWORD           i;
    unsigned long   flags;
    MYSQL           *connection;

    UNREFERENCED_PARAMETER(opaque);
    ASSERT(data != NULL);
    TRACE("opaque=%p data=%p\n", opaque, data);

    context = Io_Allocate(sizeof(DB_CONTEXT));
    if (context == NULL) {
        TRACE("Unable to allocate memory for database context.\n");

        error = ERROR_NOT_ENOUGH_MEMORY;
        goto failed;
    }
    ZeroMemory(context, sizeof(DB_CONTEXT));

    //
    // Have the MySQL client library allocate the connection structure. This is
    // in case the MYSQL structure in the headers we're compiling against changes
    // in a future version of the client library.
    //
    context->handle = mysql_init(NULL);
    if (context->handle == NULL) {
        TRACE("Unable to allocate memory for MySQL handle.\n");

        error = ERROR_NOT_ENOUGH_MEMORY;
        goto failed;
    }

    // Set connection options
    flags = CLIENT_INTERACTIVE;
    if (dbConfigServer.compression) {
        flags |= CLIENT_COMPRESS;
    }
    if (dbConfigServer.sslEnable) {
        //
        // This function always returns 0. If SSL setup is incorrect,
        // mysql_real_connect() returns an error when you attempt to connect.
        //
        mysql_ssl_set(context->handle,
            dbConfigServer.sslKeyFile,
            dbConfigServer.sslCertFile,
            dbConfigServer.sslCAFile,
            dbConfigServer.sslCAPath,
            dbConfigServer.sslCiphers);
    }

    connection = mysql_real_connect(
        context->handle,
        dbConfigServer.serverHost,
        dbConfigServer.serverUser,
        dbConfigServer.serverPass,
        dbConfigServer.serverDb,
        dbConfigServer.serverPort,
        NULL, flags);

    if (connection == NULL) {
        TRACE("Unable to connect to server: %s\n", mysql_error(context->handle));
        Io_Putlog(LOG_ERROR, "nxMyDB: Unable to connect to server: %s\r\n", mysql_error(context->handle));

        error = ERROR_CONNECTION_REFUSED;
        goto failed;
    }

    // Allocate pre-compiled statement structure
    for (i = 0; i < ELEMENT_COUNT(context->stmt); i++) {
        context->stmt[i] = mysql_stmt_init(context->handle);
        if (context->stmt[i] == NULL) {
            TRACE("Unable to allocate memory for statement structure.\n");

            error = ERROR_NOT_ENOUGH_MEMORY;
            goto failed;
        }
    }

    // Update time stamps
    GetSystemTimeAsFileTime((FILETIME *)&context->created);
    context->used = context->created;

    TRACE("Connected to %s, running MySQL Server v%s.\n",
        mysql_get_host_info(context->handle), mysql_get_server_info(context->handle));

    *data = context;
    return TRUE;

failed:
    if (context != NULL) {
        ConnectionClose(NULL, context);
    }
    SetLastError(error);
    return FALSE;
}

/*++

ConnectionCheck

    Validates the server connection.

Arguments:
    opaque  - Opaque argument passed to PoolCreate().

    data    - Pointer to the DB_CONTEXT structure.

Return Values:
    If the connection is valid, the return is nonzero (true).

    If the connection is invalid, the return is zero (false).

--*/
static BOOL FCALL ConnectionCheck(VOID *opaque, VOID *data)
{
    DB_CONTEXT *context;
    UINT64 timeCurrent;
    UINT64 timeDelta;

    UNREFERENCED_PARAMETER(opaque);
    ASSERT(data != NULL);
    TRACE("opaque=%p data=%p\n", opaque, data);

    context = data;
    GetSystemTimeAsFileTime((FILETIME *)&timeCurrent);

    // Check if the context has exceeded the expiration time
    timeDelta = timeCurrent - context->created;
    if (timeDelta > dbConfigPool.expireNano) {
        TRACE("Expiring server connection after %I64u seconds (%d second limit).\n",
            timeDelta/10000000, dbConfigPool.expire);
        SetLastError(ERROR_CONTEXT_EXPIRED);
        return FALSE;
    }

    // Check if the connection is still alive
    timeDelta = timeCurrent - context->used;
    if (timeDelta > dbConfigPool.checkNano) {
        TRACE("Connection has not been used in %I64u seconds (%d second limit), pinging it.\n",
            timeDelta/10000000, dbConfigPool.check);

        if (mysql_ping(context->handle) != 0) {
            TRACE("Lost server connection: %s\n", mysql_error(context->handle));
            SetLastError(ERROR_NOT_CONNECTED);
            return FALSE;
        }

        // Update last-use time stamp
        GetSystemTimeAsFileTime((FILETIME *)&context->used);
    }

    return TRUE;
}

/*++

ConnectionClose

    Closes the server connection.

Arguments:
    opaque  - Opaque argument passed to PoolCreate().

    data    - Pointer to the DB_CONTEXT structure.

Return Values:
    None.

--*/
static VOID FCALL ConnectionClose(VOID *opaque, VOID *data)
{
    DB_CONTEXT *context;
    DWORD      i;

    UNREFERENCED_PARAMETER(opaque);
    ASSERT(data != NULL);
    TRACE("opaque=%p data=%p\n", opaque, data);

    context = data;

    // Free MySQL structures
    for (i = 0; i < ELEMENT_COUNT(context->stmt); i++) {
        if (context->stmt[i] != NULL) {
            mysql_stmt_close(context->stmt[i]);
        }
    }
    if (context->handle != NULL) {
        mysql_close(context->handle);
    }

    // Free context structure
    Io_Free(context);
}


/*++

ConfigInit

    Initializes configuration structures.

Arguments:
    None.

Return Values:
    None.

--*/
static VOID FCALL ConfigInit(VOID)
{
    // Clear configuration structures
    ZeroMemory(&dbConfigLock,    sizeof(DB_CONFIG_LOCK));
    ZeroMemory(&dbConfigPool,    sizeof(DB_CONFIG_POOL));
    ZeroMemory(&dbConfigRefresh, sizeof(DB_CONFIG_REFRESH));
    ZeroMemory(&dbConfigServer,  sizeof(DB_CONFIG_SERVER));
}


/*++

ConfigLoad

    Loads configuration options.

Arguments:
    None.

Return Values:
    If the function succeeds, the return value is a pointer to the configuration value.

    If the function fails, the return value is null.

--*/
static BOOL FCALL ConfigLoad(VOID)
{
    //
    // Read lock options
    //

    dbConfigLock.expire = 60;
    if (Io_ConfigGetInt("nxMyDB", "Lock_Expire", &dbConfigLock.expire) && dbConfigLock.expire <= 0) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Lock_Expire' must be greater than zero.\r\n");
        return FALSE;
    }

    dbConfigLock.timeout = 5;
    if (Io_ConfigGetInt("nxMyDB", "Lock_Timeout", &dbConfigLock.timeout) && dbConfigLock.timeout <= 0) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Lock_Timeout' must be greater than zero.\r\n");
        return FALSE;
    }

    //
    // Read pool options
    //

    dbConfigPool.minimum = 1;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Minimum", &dbConfigPool.minimum) && dbConfigPool.minimum <= 0) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Pool_Minimum' must be greater than zero.\r\n");
        return FALSE;
    }

    dbConfigPool.average = dbConfigPool.minimum + 1;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Average", &dbConfigPool.average) && dbConfigPool.average < dbConfigPool.minimum) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Pool_Average' must be greater than or equal to 'Pool_Minimum'.\r\n");
        return FALSE;
    }

    dbConfigPool.maximum = dbConfigPool.average * 2;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Maximum", &dbConfigPool.maximum) && dbConfigPool.maximum < dbConfigPool.average) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Pool_Maximum' must be greater than or equal to 'Pool_Average'.\r\n");
        return FALSE;
    }

    dbConfigPool.timeout = 5;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Timeout", &dbConfigPool.timeout) && dbConfigPool.timeout <= 0) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Pool_Timeout' must be greater than zero.\r\n");
        return FALSE;
    }
    dbConfigPool.timeoutMili = dbConfigPool.timeout * 1000; // sec to msec

    dbConfigPool.expire = 3600;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Expire", &dbConfigPool.expire) && dbConfigPool.expire <= 0) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Pool_Expire' must be greater than zero.\r\n");
        return FALSE;
    }
    dbConfigPool.expireNano = UInt32x32To64(dbConfigPool.expire, 10000000); // sec to 100nsec

    dbConfigPool.check = 60;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Check", &dbConfigPool.check) &&
            (dbConfigPool.check <= 0 || dbConfigPool.check >= dbConfigPool.expire)) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Pool_Check' must be greater than zero and less than 'Pool_Expire'.\r\n");
        return FALSE;
    }
    dbConfigPool.checkNano = UInt32x32To64(dbConfigPool.check, 10000000); // sec to 100nsec

    //
    // Read refesh timer
    //

    dbConfigRefresh.interval = 0;
    if (Io_ConfigGetInt("nxMyDB", "Refresh", &dbConfigRefresh.interval) && dbConfigRefresh.interval < 0) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Refresh' must be greater than or equal to zero.\r\n");
        return FALSE;
    }
    dbConfigRefresh.intervalMili = dbConfigRefresh.interval * 1000; // sec to msec

    //
    // Read server options
    //

    dbConfigServer.serverHost = ConfigGet("nxMyDB", "Host");
    dbConfigServer.serverUser = ConfigGet("nxMyDB", "User");
    dbConfigServer.serverPass = ConfigGet("nxMyDB", "Password");
    dbConfigServer.serverDb   = ConfigGet("nxMyDB", "Database");
    Io_ConfigGetInt("nxMyDB", "Port", &dbConfigServer.serverPort);
    Io_ConfigGetBool("nxMyDB", "Compression", &dbConfigServer.compression);
    Io_ConfigGetBool("nxMyDB", "SSL_Enable", &dbConfigServer.sslEnable);
    dbConfigServer.sslCiphers  = ConfigGet("nxMyDB", "SSL_Ciphers");
    dbConfigServer.sslCertFile = ConfigGet("nxMyDB", "SSL_Cert_File");
    dbConfigServer.sslKeyFile  = ConfigGet("nxMyDB", "SSL_Key_File");
    dbConfigServer.sslCAFile   = ConfigGet("nxMyDB", "SSL_CA_File");
    dbConfigServer.sslCAPath   = ConfigGet("nxMyDB", "SSL_CA_Path");

    return TRUE;
}

/*++

ConfigFree

    Frees memory allocated for configuration options.

Arguments:
    None.

Return Values:
    None.

--*/
static VOID FCALL ConfigFree(VOID)
{
    // Free server options
    if (dbConfigServer.serverHost != NULL) {
        Io_Free(dbConfigServer.serverHost);
    }
    if (dbConfigServer.serverUser != NULL) {
        Io_Free(dbConfigServer.serverUser);
    }
    if (dbConfigServer.serverPass != NULL) {
        Io_Free(dbConfigServer.serverPass);
    }
    if (dbConfigServer.serverDb != NULL) {
        Io_Free(dbConfigServer.serverDb);
    }

    // Free SSL options
    if (dbConfigServer.sslCiphers != NULL) {
        Io_Free(dbConfigServer.sslCiphers);
    }
    if (dbConfigServer.sslCertFile != NULL) {
        Io_Free(dbConfigServer.sslCertFile);
    }
    if (dbConfigServer.sslKeyFile != NULL) {
        Io_Free(dbConfigServer.sslKeyFile);
    }
    if (dbConfigServer.sslCAFile != NULL) {
        Io_Free(dbConfigServer.sslCAFile);
    }
    if (dbConfigServer.sslCAPath != NULL) {
        Io_Free(dbConfigServer.sslCAPath);
    }

    // Clear configuration structures
    ZeroMemory(&dbConfigLock,    sizeof(DB_CONFIG_LOCK));
    ZeroMemory(&dbConfigPool,    sizeof(DB_CONFIG_POOL));
    ZeroMemory(&dbConfigRefresh, sizeof(DB_CONFIG_REFRESH));
    ZeroMemory(&dbConfigServer,  sizeof(DB_CONFIG_SERVER));
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
static CHAR *FCALL ConfigGet(CHAR *array, CHAR *variable)
{
    CHAR *p;
    CHAR *value;
    SIZE_T length;

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

ConfigUuid

    Generates a UUID used for identifying the server.

Arguments:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

Return Values:
    None.

--*/
static BOOL FCALL ConfigUuid(VOID)
{
    CHAR        *format;
    RPC_STATUS  status;
    UUID        uuid;

    status = UuidCreate(&uuid);
    switch (status) {
        case RPC_S_OK:
        case RPC_S_UUID_LOCAL_ONLY:
        case RPC_S_UUID_NO_ADDRESS:
            // These are acceptable failures.
            break;
        default:
            TRACE("Unable to generate UUID (error %lu).\n", status);
            return FALSE;
    }

    status = UuidToStringA(&uuid, (RPC_CSTR *)&format);
    if (status != RPC_S_OK) {
            TRACE("Unable to convert UUID (error %lu).\n", status);
        return FALSE;
    }

    // Copy formatted UUID
    StringCchCopyA(dbConfigLock.owner, ELEMENT_COUNT(dbConfigLock.owner), format);
    dbConfigLock.ownerLength = strlen(dbConfigLock.owner);

    TRACE("UUID is %s\n", format);

    RpcStringFree((RPC_CSTR *)&format);
    return TRUE;
}


/*++

RefreshTimer

    Refreshes the local user and group cache.

Arguments:
    notUsed     - Pointer to the timer context.

    currTimer   - Pointer to the current TIMER structure.

Return Values:
    Number of milliseconds to execute this timer again.

--*/
static DWORD RefreshTimer(VOID *notUsed, TIMER *currTimer)
{
    DB_CONTEXT *context;

    UNREFERENCED_PARAMETER(notUsed);
    UNREFERENCED_PARAMETER(currTimer);

    TRACE("currTimer=%p", currTimer);

    if (DbAcquire(&context)) {
        //
        // Groups must be updated first - since user
        // files have dependencies on group files.
        //
        DbGroupRefresh(context);
        DbUserRefresh(context);

        DbRelease(context);
    } else {
        TRACE("Unable to acquire a database connection.\n");
    }

    // Execute the timer again
    return dbConfigRefresh.intervalMili;
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
BOOL FCALL DbInit(Io_GetProc *getProc)
{
    BOOL result;

    TRACE("refCount=%d\n", refCount);

    // Only initialize the database pool once
    if (refCount++) {
        TRACE("Already initialized, returning.\n");
        return TRUE;
    }

    // Initialize configuration structures
    ConfigInit();

    // Initialize procedure table
    if (!ProcTableInit(getProc)) {
        TRACE("Unable to initialize procedure table.\n");
        return FALSE;
    }

    // Load configuration options
    if (!ConfigLoad()) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Configuraiton error.\r\n");

        DbFinalize();
        return FALSE;
    }

    // Generate a UUID for this server
    if (!ConfigUuid()) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Unable to generate UUID.\r\n");

        DbFinalize();
        return FALSE;
    }

    // Create connection pool
    pool = Io_Allocate(sizeof(POOL));
    if (pool == NULL) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Unable to allocate memory for connection pool.\r\n");

        DbFinalize();
        return FALSE;
    }

    result = PoolCreate(pool,
        dbConfigPool.minimum, dbConfigPool.average,
        dbConfigPool.maximum, dbConfigPool.timeoutMili,
        ConnectionOpen, ConnectionCheck, ConnectionClose, NULL);

    if (!result) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Unable to initialize connection pool.\r\n");

        DbFinalize();
        return FALSE;
    }

    // Start database refresh timer
    if (dbConfigRefresh.intervalMili > 0) {
        dbConfigRefresh.timer = Io_StartIoTimer(NULL, RefreshTimer, NULL, dbConfigRefresh.intervalMili);
    }

    Io_Putlog(LOG_ERROR, "nxMyDB: v%s loaded, using MySQL Client Library v%s.\r\n",
        STRINGIFY(VERSION), mysql_get_client_info());
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
VOID FCALL DbFinalize(VOID)
{
    TRACE("refCount=%d\n", refCount);

    // Finalize once the reference count reaches zero
    if (--refCount == 0) {

        // Stop refresh timer
        if (dbConfigRefresh.timer != NULL) {
            Io_StopIoTimer(dbConfigRefresh.timer, FALSE);
        }

        // Destroy connection pool
        PoolDestroy(pool);
        Io_Free(pool);

        // Notify user
        Io_Putlog(LOG_ERROR, "nxMyDB: v%s unloaded.\r\n", STRINGIFY(VERSION));

        ConfigFree();
        ProcTableFinalize();
    }
}

/*++

DbMapError

    Maps a MySQL result code to the closest Windows error code.

Arguments:
    error   - MySQL client library error code.

Return Values:
    The closest Windows error code.

--*/
DWORD FCALL DbMapError(UINT error)
{
    DWORD result;

    switch (error) {
        case 0:
            result = ERROR_SUCCESS;
            break;

        case CR_COMMANDS_OUT_OF_SYNC:
        case CR_NOT_IMPLEMENTED:
            result = ERROR_INTERNAL_ERROR;
            break;

        case CR_OUT_OF_MEMORY:
            result = ERROR_NOT_ENOUGH_MEMORY;
            break;

        case CR_UNKNOWN_ERROR:
            result = ERROR_INVALID_FUNCTION;
            break;

        case CR_SERVER_GONE_ERROR:
        case CR_SERVER_LOST:
        case CR_SERVER_LOST_EXTENDED:
            result = ERROR_NOT_CONNECTED;
            break;

        case CR_PARAMS_NOT_BOUND:
        case CR_NO_PARAMETERS_EXISTS:
        case CR_INVALID_PARAMETER_NO:
        case CR_INVALID_BUFFER_USE:
        case CR_UNSUPPORTED_PARAM_TYPE:
            result = ERROR_INVALID_PARAMETER;
            break;

        default:
            TRACE("Unknown MySQL result error %lu.\n", error);
            result = ERROR_INVALID_FUNCTION;
    }

    return result;
}

/*++

DbAcquire

    Acquires a database context from the connection pool.

Arguments:
    dbContext   - Pointer to a pointer that receives the DB_CONTEXT structure.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
BOOL FCALL DbAcquire(DB_CONTEXT **dbContext)
{
    DB_CONTEXT *context;

    ASSERT(dbContext != NULL);
    TRACE("dbContext=%p\n", dbContext);

    // Acquire a database context
    if (!PoolAcquire(pool, &context)) {
        TRACE("Unable to acquire a database context (error %lu).\n", GetLastError());
        return FALSE;
    }

    *dbContext = context;
    return TRUE;
}

/*++

DbRelease

    Releases a database context back into the connection pool.

Arguments:
    dbContext   - Pointer to the DB_CONTEXT structure.

Return Values:
    None.

--*/
VOID FCALL DbRelease(DB_CONTEXT *dbContext)
{
    ASSERT(dbContext != NULL);
    TRACE("dbContext=%p\n", dbContext);

    // Update last-use time stamp
    GetSystemTimeAsFileTime((FILETIME *)&dbContext->used);

    // Release the database context
    if (!PoolRelease(pool, dbContext)) {
        TRACE("Unable to release the database context (error %lu).\n", GetLastError());
    }
}
