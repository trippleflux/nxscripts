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

// Pool resource functions
static POOL_CONSTRUCTOR_PROC ConnectionOpen;
static POOL_VALIDATOR_PROC   ConnectionCheck;
static POOL_DESTRUCTOR_PROC  ConnectionClose;

// MySQL Server information
static CHAR *serverHost;
static CHAR *serverUser;
static CHAR *serverPass;
static CHAR *serverDb;
static INT   serverPort;
static BOOL  compression;
static BOOL  sslEnable;
static CHAR *sslCiphers;
static CHAR *sslCertFile;
static CHAR *sslKeyFile;
static CHAR *sslCAFile;
static CHAR *sslCAPath;

// Connection expiration
static UINT64 connCheck;
static UINT64 connExpire;

// Locking
static INT  lockExpire;
static INT  lockTimeout;
static CHAR lockOwner[36 + 1];

// Refresh timer
static INT refresh;
static TIMER *timer = NULL;

// Database connection pool
static POOL *pool;

// Reference count initialization calls
static INT refCount = 0;


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
static BOOL ConnectionOpen(VOID *opaque, VOID **data)
{
    DB_CONTEXT      *context;
    DWORD           error;
    unsigned long   flags;

    ASSERT(opaque == NULL);
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
    if (compression) {
        flags |= CLIENT_COMPRESS;
    }
    if (sslEnable) {
        //
        // This function always returns 0. If SSL setup is incorrect,
        // mysql_real_connect() returns an error when you attempt to connect.
        //
        mysql_ssl_set(context->handle, sslKeyFile, sslCertFile, sslCAFile, sslCAPath, sslCiphers);
    }

    if (!mysql_real_connect(context->handle, serverHost, serverUser, serverPass, serverDb, serverPort, NULL, flags)) {
        TRACE("Unable to connect to server: %s\n", mysql_error(context->handle));
        Io_Putlog(LOG_ERROR, "nxMyDB: Unable to connect to server: %s\r\n", mysql_error(context->handle));

        error = ERROR_CONNECTION_REFUSED;
        goto failed;
    }

    // Allocate pre-compiled statement structure
    context->stmt = mysql_stmt_init(context->handle);
    if (context->stmt == NULL) {
        TRACE("Unable to allocate memory for statement structure.\n");

        error = ERROR_NOT_ENOUGH_MEMORY;
        goto failed;
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
static BOOL ConnectionCheck(VOID *opaque, VOID *data)
{
    DB_CONTEXT *context;
    UINT64 timeCurrent;
    UINT64 timeDelta;

    ASSERT(opaque == NULL);
    ASSERT(data != NULL);
    TRACE("opaque=%p data=%p\n", opaque, data);

    context = data;
    GetSystemTimeAsFileTime((FILETIME *)&timeCurrent);

    // Check if the context has exceeded the expiration time
    timeDelta = timeCurrent - context->created;
    if (timeDelta > connExpire) {
        TRACE("Expiring server connection after %I64u seconds (%I64u second limit).\n",
            timeDelta/10000000, connExpire/10000000);
        SetLastError(ERROR_CONTEXT_EXPIRED);
        return FALSE;
    }

    // Check if the connection is still alive
    timeDelta = timeCurrent - context->used;
    if (timeDelta > connCheck) {
        TRACE("Connection has not been used in %I64u seconds (%I64u second limit), pinging it.\n",
            timeDelta/10000000, connCheck/10000000);

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
static VOID ConnectionClose(VOID *opaque, VOID *data)
{
    DB_CONTEXT *context;

    ASSERT(opaque == NULL);
    ASSERT(data != NULL);
    TRACE("opaque=%p data=%p\n", opaque, data);

    context = data;

    // Free MySQL structures
    if (context->stmt != NULL) {
        mysql_stmt_close(context->stmt);
    }
    if (context->handle != NULL) {
        mysql_close(context->handle);
    }

    // Free context structure
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
static CHAR *ConfigGet(CHAR *array, CHAR *variable)
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
static BOOL ConfigUuid(VOID)
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

    // Copy formatted UUID to the lock owner buffer
    StringCchCopyA(lockOwner, ELEMENT_COUNT(lockOwner), format);

    TRACE("UUID is %s\n", format);

    RpcStringFree((RPC_CSTR *)&format);
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
static VOID ConfigFree(VOID)
{
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
BOOL DbInit(Io_GetProc *getProc)
{
    INT poolMin;
    INT poolAvg;
    INT poolMax;
    INT poolCheck;
    INT poolExpire;
    INT poolTimeout;

    TRACE("refCount=%d\n", refCount);

    // Only initialize the database pool once
    if (refCount++) {
        TRACE("Already initialized, returning.\n");
        return TRUE;
    }

    // Initialize procedure table
    if (!ProcTableInit(getProc)) {
        TRACE("Unable to initialize procedure table.\n");
        return FALSE;
    }

    //
    // Read lock options
    //

    lockExpire = 60;
    if (Io_ConfigGetInt("nxMyDB", "Lock_Expire", &lockExpire) && lockExpire <= 0) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Lock_Expire' must be greater than zero.\r\n");
        return FALSE;
    }

    lockTimeout = 5;
    if (Io_ConfigGetInt("nxMyDB", "Lock_Timeout", &lockTimeout) && lockTimeout <= 0) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Lock_Timeout' must be greater than zero.\r\n");
        return FALSE;
    }

    //
    // Read pool options
    //

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
    poolTimeout *= 1000; // sec to msec

    poolExpire = 3600;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Expire", &poolExpire) && poolExpire <= 0) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Pool_Expire' must be greater than zero.\r\n");
        return FALSE;
    }
    connExpire = UInt32x32To64(poolExpire, 10000000); // sec to 100nsec

    poolCheck = 60;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Check", &poolCheck) && (poolCheck <= 0 || poolCheck >= poolExpire)) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Pool_Check' must be greater than zero and less than 'Pool_Expire'.\r\n");
        return FALSE;
    }
    connCheck = UInt32x32To64(poolCheck, 10000000); // sec to 100nsec

    //
    // Read refesh timer
    //

    refresh = 0;
    if (Io_ConfigGetInt("nxMyDB", "Refresh", &refresh) && refresh < 0) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Option 'Refresh' must be greater than or equal to zero.\r\n");
        return FALSE;
    }
    refresh *= 1000; // sec to msec

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

    // Create connection pool
    pool = Io_Allocate(sizeof(POOL));
    if (pool == NULL) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Unable to allocate memory for connection pool.\r\n");

        ConfigFree();
        return FALSE;
    }
    if (!PoolCreate(pool, poolMin, poolAvg, poolMax, poolTimeout,
            ConnectionOpen, ConnectionCheck, ConnectionClose, NULL)) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Unable to initialize connection pool.\r\n");

        Io_Free(pool);
        ConfigFree();
        return FALSE;
    }

    // Generate a UUID for this server
    if (!ConfigUuid()) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Unable to generate UUID.\r\n");
        DbFinalize();
        return FALSE;
    }

    // Start database refresh timer
    if (refresh > 0) {
        timer = Io_StartIoTimer(NULL, RefreshTimer, NULL, refresh);
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
VOID DbFinalize(VOID)
{
    TRACE("refCount=%d\n", refCount);

    // Finalize once the reference count reaches zero
    if (--refCount == 0) {

        // Stop refresh timer
        if (timer != NULL) {
            Io_StopIoTimer(timer, FALSE);
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

DbGetConfig

    Retrieves the lock configuration.

Arguments:
    expire  - Pointer to a variable that recieves the lock expiration.

    timeout - Pointer to a variable that recieves the lock timeout.

    owner   - Pointer to a variable that recieves the lock owner.

Return Values:
    None.

--*/
VOID DbGetConfig(INT *expire, INT *timeout, CHAR **owner)
{

    if (expire != NULL) {
        *expire = lockExpire;
    }
    if (timeout != NULL) {
        *timeout = lockTimeout;
    }
    if (owner != NULL) {
        *owner = lockOwner;
    }
}

/*++

DbMapError

    Maps a MySQL result code to the closest Windows error code.

Arguments:
    result  - MySQL client library result code.

Return Values:
    The closest Windows error code.

--*/
DWORD DbMapError(INT result)
{
    DWORD error;

    switch (result) {
        case 0:
            error = ERROR_SUCCESS;
            break;

        case CR_COMMANDS_OUT_OF_SYNC:
        case CR_NOT_IMPLEMENTED:
            error = ERROR_INTERNAL_ERROR;
            break;

        case CR_OUT_OF_MEMORY:
            error = ERROR_NOT_ENOUGH_MEMORY;
            break;

        case CR_UNKNOWN_ERROR:
            error = ERROR_INVALID_FUNCTION;
            break;

        case CR_SERVER_GONE_ERROR:
        case CR_SERVER_LOST:
        case CR_SERVER_LOST_EXTENDED:
            error = ERROR_NOT_CONNECTED;
            break;

        case CR_PARAMS_NOT_BOUND:
        case CR_NO_PARAMETERS_EXISTS:
        case CR_INVALID_PARAMETER_NO:
        case CR_INVALID_BUFFER_USE:
        case CR_UNSUPPORTED_PARAM_TYPE:
            error = ERROR_INVALID_PARAMETER;
            break;

        default:
            TRACE("Unknown MySQL result code %d.\n", result);
            error = ERROR_INVALID_FUNCTION;
    }

    return error;
}

/*++

DbMapMessage

    Maps a MySQL result code to an error string.

Arguments:
    result  - MySQL client library result code.

Return Values:
    The error message.

--*/
const CHAR *DbMapMessage(INT result)
{
    switch (result) {
        case CR_UNKNOWN_ERROR:            return "CR_UNKNOWN_ERROR";
        case CR_SOCKET_CREATE_ERROR:      return "CR_SOCKET_CREATE_ERROR";
        case CR_CONNECTION_ERROR:         return "CR_CONNECTION_ERROR";
        case CR_CONN_HOST_ERROR:          return "CR_CONN_HOST_ERROR";
        case CR_IPSOCK_ERROR:             return "CR_IPSOCK_ERROR";
        case CR_UNKNOWN_HOST:             return "CR_UNKNOWN_HOST";
        case CR_SERVER_GONE_ERROR:        return "CR_SERVER_GONE_ERROR";
        case CR_VERSION_ERROR:            return "CR_VERSION_ERROR";
        case CR_OUT_OF_MEMORY:            return "CR_OUT_OF_MEMORY";
        case CR_WRONG_HOST_INFO:          return "CR_WRONG_HOST_INFO";
        case CR_LOCALHOST_CONNECTION:     return "CR_LOCALHOST_CONNECTION";
        case CR_TCP_CONNECTION:           return "CR_TCP_CONNECTION";
        case CR_SERVER_HANDSHAKE_ERR:     return "CR_SERVER_HANDSHAKE_ERR";
        case CR_SERVER_LOST:              return "CR_SERVER_LOST";
        case CR_COMMANDS_OUT_OF_SYNC:     return "CR_COMMANDS_OUT_OF_SYNC";
        case CR_NAMEDPIPE_CONNECTION:     return "CR_NAMEDPIPE_CONNECTION";
        case CR_NAMEDPIPEWAIT_ERROR:      return "CR_NAMEDPIPEWAIT_ERROR";
        case CR_NAMEDPIPEOPEN_ERROR:      return "CR_NAMEDPIPEOPEN_ERROR";
        case CR_NAMEDPIPESETSTATE_ERROR:  return "CR_NAMEDPIPESETSTATE_ERROR";
        case CR_CANT_READ_CHARSET:        return "CR_CANT_READ_CHARSET";
        case CR_NET_PACKET_TOO_LARGE:     return "CR_NET_PACKET_TOO_LARGE";
        case CR_EMBEDDED_CONNECTION:      return "CR_EMBEDDED_CONNECTION";
        case CR_PROBE_SLAVE_STATUS:       return "CR_PROBE_SLAVE_STATUS";
        case CR_PROBE_SLAVE_HOSTS:        return "CR_PROBE_SLAVE_HOSTS";
        case CR_PROBE_SLAVE_CONNECT:      return "CR_PROBE_SLAVE_CONNECT";
        case CR_PROBE_MASTER_CONNECT:     return "CR_PROBE_MASTER_CONNECT";
        case CR_SSL_CONNECTION_ERROR:     return "CR_SSL_CONNECTION_ERROR";
        case CR_MALFORMED_PACKET:         return "CR_MALFORMED_PACKET";
        case CR_WRONG_LICENSE:            return "CR_WRONG_LICENSE";
        case CR_NULL_POINTER:             return "CR_NULL_POINTER";
        case CR_NO_PREPARE_STMT:          return "CR_NO_PREPARE_STMT";
        case CR_PARAMS_NOT_BOUND:         return "CR_PARAMS_NOT_BOUND";
        case CR_DATA_TRUNCATED:           return "CR_DATA_TRUNCATED";
        case CR_NO_PARAMETERS_EXISTS:     return "CR_NO_PARAMETERS_EXISTS";
        case CR_INVALID_PARAMETER_NO:     return "CR_INVALID_PARAMETER_NO";
        case CR_INVALID_BUFFER_USE:       return "CR_INVALID_BUFFER_USE";
        case CR_UNSUPPORTED_PARAM_TYPE:   return "CR_UNSUPPORTED_PARAM_TYPE";
        case CR_SHARED_MEMORY_CONNECTION: return "CR_SHARED_MEMORY_CONNECTION";
        case CR_SHARED_MEMORY_CONNECT_REQUEST_ERROR:   return "CR_SHARED_MEMORY_CONNECT_REQUEST_ERROR";
        case CR_SHARED_MEMORY_CONNECT_ANSWER_ERROR:    return "CR_SHARED_MEMORY_CONNECT_ANSWER_ERROR";
        case CR_SHARED_MEMORY_CONNECT_FILE_MAP_ERROR:  return "CR_SHARED_MEMORY_CONNECT_FILE_MAP_ERROR";
        case CR_SHARED_MEMORY_CONNECT_MAP_ERROR:       return "CR_SHARED_MEMORY_CONNECT_MAP_ERROR";
        case CR_SHARED_MEMORY_FILE_MAP_ERROR:          return "CR_SHARED_MEMORY_FILE_MAP_ERROR";
        case CR_SHARED_MEMORY_MAP_ERROR:               return "CR_SHARED_MEMORY_MAP_ERROR";
        case CR_SHARED_MEMORY_EVENT_ERROR:             return "CR_SHARED_MEMORY_EVENT_ERROR";
        case CR_SHARED_MEMORY_CONNECT_ABANDONED_ERROR: return "CR_SHARED_MEMORY_CONNECT_ABANDONED_ERROR";
        case CR_SHARED_MEMORY_CONNECT_SET_ERROR:       return "CR_SHARED_MEMORY_CONNECT_SET_ERROR";
        case CR_CONN_UNKNOW_PROTOCOL:     return "CR_CONN_UNKNOW_PROTOCOL";
        case CR_INVALID_CONN_HANDLE:      return "CR_INVALID_CONN_HANDLE";
        case CR_SECURE_AUTH:              return "CR_SECURE_AUTH";
        case CR_FETCH_CANCELED:           return "CR_FETCH_CANCELED";
        case CR_NO_DATA:                  return "CR_NO_DATA";
        case CR_NO_STMT_METADATA:         return "CR_NO_STMT_METADATA";
        case CR_NO_RESULT_SET:            return "CR_NO_RESULT_SET";
        case CR_NOT_IMPLEMENTED:          return "CR_NOT_IMPLEMENTED";
        case CR_SERVER_LOST_EXTENDED:     return "CR_SERVER_LOST_EXTENDED";
    }

    return "Unknown result code.";
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
BOOL DbAcquire(DB_CONTEXT **dbContext)
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
VOID DbRelease(DB_CONTEXT *dbContext)
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
