/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2008 neoxed

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
#include <logging.h>
#include <pool.h>

#include <errmsg.h>
#include <rpc.h>


//
// Configuration structures
//
typedef struct {
    INT     logLevel;       // Level of log verbosity
} DB_CONFIG_GLOBAL;

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
    BOOL         enabled;   // Allow syncronization
    INT          first;     // Milliseconds until the first syncronization
    INT          interval;  // Milliseconds between each database refresh
    SYNC_CONTEXT sync;      // Syncronization context
    TIMER        *timer;    // Syncronization timer
} DB_CONFIG_SYNC;

//
// Configuration variables
//

DB_CONFIG_LOCK dbConfigLock;

static DB_CONFIG_GLOBAL  dbConfigGlobal;
static DB_CONFIG_POOL    dbConfigPool;
static DB_CONFIG_SERVER  dbConfigServer;
static DB_CONFIG_SYNC    dbConfigSync;

static POOL pool;           // Database connection pool
static INT  refCount = 0;   // Reference count initialization calls

//
// Function declarations
//

static POOL_CONSTRUCTOR_PROC ConnectionOpen;
static POOL_VALIDATOR_PROC   ConnectionCheck;
static POOL_DESTRUCTOR_PROC  ConnectionClose;

static DWORD FCALL ConfigInit(VOID);
static BOOL  FCALL ConfigLoad(VOID);
static DWORD FCALL ConfigFree(VOID);

static CHAR *FCALL ConfigGet(CHAR *array, CHAR *variable);
static DWORD FCALL ConfigUuid(VOID);


/*++

ConnectionOpen

    Opens a server connection.

Arguments:
    context - Opaque context passed to <PoolCreate>.

    data    - Pointer to a pointer that receives the DB_CONTEXT structure.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
static BOOL FCALL ConnectionOpen(VOID *context, VOID **data)
{
    DB_CONTEXT  *db;
    DWORD       error;
    DWORD       i;
    MYSQL       *connection;
    ULONG       flags;

    UNREFERENCED_PARAMETER(context);
    ASSERT(data != NULL);
    TRACE("context=%p data=%p", context, data);

    db = MemAllocate(sizeof(DB_CONTEXT));
    if (db == NULL) {
        LOG_ERROR("Unable to allocate memory for database context.");

        error = ERROR_NOT_ENOUGH_MEMORY;
        goto failed;
    }
    ZeroMemory(db, sizeof(DB_CONTEXT));

    //
    // Have the MySQL client library allocate the connection structure. This is
    // in case the MYSQL structure in the headers we're compiling against changes
    // in a future version of the client library.
    //
    db->handle = mysql_init(NULL);
    if (db->handle == NULL) {
        LOG_ERROR("Unable to allocate memory for MySQL handle.");

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
        mysql_ssl_set(db->handle,
            dbConfigServer.sslKeyFile,
            dbConfigServer.sslCertFile,
            dbConfigServer.sslCAFile,
            dbConfigServer.sslCAPath,
            dbConfigServer.sslCiphers);
    }

    connection = mysql_real_connect(
        db->handle,
        dbConfigServer.host,
        dbConfigServer.user,
        dbConfigServer.password,
        dbConfigServer.database,
        dbConfigServer.port,
        NULL, flags);

    if (connection == NULL) {
        LOG_ERROR("Unable to connect to server: %s", mysql_error(db->handle));

        error = ERROR_CONNECTION_REFUSED;
        goto failed;
    }

    // Pointer values should be the same
    ASSERT(connection == db->handle);

    // Check server version
    if (mysql_get_server_version(db->handle) < 50019) {
        LOG_ERROR("Unsupported version of MySQL Server - you are running v%s, must be v5.0.19 or newer.",
            mysql_get_server_info(db->handle));

        error = ERROR_NOT_SUPPORTED;
        goto failed;
    }

    // Allocate pre-compiled statement structure
    for (i = 0; i < ELEMENT_COUNT(db->stmt); i++) {
        db->stmt[i] = mysql_stmt_init(db->handle);
        if (db->stmt[i] == NULL) {
            LOG_ERROR("Unable to allocate memory for statement structure.");

            error = ERROR_NOT_ENOUGH_MEMORY;
            goto failed;
        }
    }

    // Update time stamps
    GetSystemTimeAsFileTime((FILETIME *)&db->created);
    db->used = db->created;

    LOG_INFO("Connected to %s, running MySQL Server v%s.",
        mysql_get_host_info(db->handle), mysql_get_server_info(db->handle));

    *data = db;
    return TRUE;

failed:
    if (db != NULL) {
        ConnectionClose(NULL, db);
    }
    SetLastError(error);
    return FALSE;
}

/*++

ConnectionCheck

    Validates the server connection.

Arguments:
    context - Opaque context passed to <PoolCreate>.

    data    - Pointer to the DB_CONTEXT structure.

Return Values:
    If the connection is valid, the return is nonzero (true).

    If the connection is invalid, the return is zero (false).

--*/
static BOOL FCALL ConnectionCheck(VOID *context, VOID *data)
{
    DB_CONTEXT  *db = data;
    UINT64      timeCurrent;
    UINT64      timeDelta;

    UNREFERENCED_PARAMETER(context);
    ASSERT(data != NULL);
    TRACE("context=%p data=%p", context, data);

    GetSystemTimeAsFileTime((FILETIME *)&timeCurrent);

    // Check if the context has exceeded the expiration time
    timeDelta = timeCurrent - db->created;
    if (timeDelta > dbConfigPool.expireNano) {
        LOG_INFO("Expiring server connection after %I64u seconds (%d second limit).",
            timeDelta/10000000, dbConfigPool.expire);
        SetLastError(ERROR_CONTEXT_EXPIRED);
        return FALSE;
    }

    // Check if the connection is still alive
    timeDelta = timeCurrent - db->used;
    if (timeDelta > dbConfigPool.checkNano) {
        LOG_INFO("Connection has not been used in %I64u seconds (%d second limit), pinging it.",
            timeDelta/10000000, dbConfigPool.check);

        if (mysql_ping(db->handle) != 0) {
            LOG_WARN("Lost server connection: %s", mysql_error(db->handle));
            SetLastError(ERROR_NOT_CONNECTED);
            return FALSE;
        }

        // Update last-use time stamp
        GetSystemTimeAsFileTime((FILETIME *)&db->used);
    }

    return TRUE;
}

/*++

ConnectionClose

    Closes the server connection.

Arguments:
    context - Opaque context passed to <PoolCreate>.

    data    - Pointer to the DB_CONTEXT structure.

Return Values:
    None.

--*/
static VOID FCALL ConnectionClose(VOID *context, VOID *data)
{
    DB_CONTEXT  *db = data;
    DWORD       i;

    UNREFERENCED_PARAMETER(context);
    ASSERT(data != NULL);
    TRACE("context=%p data=%p", context, data);

    // Free MySQL structures
    for (i = 0; i < ELEMENT_COUNT(db->stmt); i++) {
        if (db->stmt[i] != NULL) {
            mysql_stmt_close(db->stmt[i]);
        }
    }
    if (db->handle != NULL) {
        mysql_close(db->handle);
    }

    // Free context structure
    MemFree(db);
}


/*++

ConfigInit

    Initializes configuration structures.

Arguments:
    None.

Return Values:
    A Windows API error code.

--*/
static DWORD FCALL ConfigInit(VOID)
{
    // Clear configuration structures
    ZeroMemory(&dbConfigGlobal, sizeof(DB_CONFIG_GLOBAL));
    ZeroMemory(&dbConfigLock,   sizeof(DB_CONFIG_LOCK));
    ZeroMemory(&dbConfigPool,   sizeof(DB_CONFIG_POOL));
    ZeroMemory(&dbConfigServer, sizeof(DB_CONFIG_SERVER));
    ZeroMemory(&dbConfigSync,   sizeof(DB_CONFIG_SYNC));
    return ERROR_SUCCESS;
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
    // Read global options
    //

    dbConfigGlobal.logLevel = (INT)LOG_LEVEL_ERROR;
    Io_ConfigGetInt("nxMyDB", "Log_Level", &dbConfigGlobal.logLevel);

    //
    // Read lock options
    //

    dbConfigLock.expire = 60;
    if (Io_ConfigGetInt("nxMyDB", "Lock_Expire", &dbConfigLock.expire) && dbConfigLock.expire <= 0) {
        LOG_ERROR("Option 'Lock_Expire' must be greater than zero.");
        return FALSE;
    }

    dbConfigLock.timeout = 5;
    if (Io_ConfigGetInt("nxMyDB", "Lock_Timeout", &dbConfigLock.timeout) && dbConfigLock.timeout <= 0) {
        LOG_ERROR("Option 'Lock_Timeout' must be greater than zero.");
        return FALSE;
    }

    //
    // Read pool options
    //

    dbConfigPool.minimum = 1;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Minimum", &dbConfigPool.minimum) && dbConfigPool.minimum <= 0) {
        LOG_ERROR("Option 'Pool_Minimum' must be greater than zero.");
        return FALSE;
    }

    dbConfigPool.average = dbConfigPool.minimum + 1;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Average", &dbConfigPool.average) && dbConfigPool.average < dbConfigPool.minimum) {
        LOG_ERROR("Option 'Pool_Average' must be greater than or equal to 'Pool_Minimum'.");
        return FALSE;
    }

    dbConfigPool.maximum = dbConfigPool.average * 2;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Maximum", &dbConfigPool.maximum) && dbConfigPool.maximum < dbConfigPool.average) {
        LOG_ERROR("Option 'Pool_Maximum' must be greater than or equal to 'Pool_Average'.");
        return FALSE;
    }

    dbConfigPool.timeout = 5;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Timeout", &dbConfigPool.timeout) && dbConfigPool.timeout <= 0) {
        LOG_ERROR("Option 'Pool_Timeout' must be greater than zero.");
        return FALSE;
    }
    dbConfigPool.timeoutMili = dbConfigPool.timeout * 1000; // sec to msec

    dbConfigPool.expire = 3600;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Expire", &dbConfigPool.expire) && dbConfigPool.expire <= 0) {
        LOG_ERROR("Option 'Pool_Expire' must be greater than zero.");
        return FALSE;
    }
    dbConfigPool.expireNano = UInt32x32To64(dbConfigPool.expire, 10000000); // sec to 100nsec

    dbConfigPool.check = 60;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Check", &dbConfigPool.check) &&
            (dbConfigPool.check <= 0 || dbConfigPool.check >= dbConfigPool.expire)) {
        LOG_ERROR("Option 'Pool_Check' must be greater than zero and less than 'Pool_Expire'.");
        return FALSE;
    }
    dbConfigPool.checkNano = UInt32x32To64(dbConfigPool.check, 10000000); // sec to 100nsec

    //
    // Read sync options
    //

    Io_ConfigGetBool("nxMyDB", "Sync", &dbConfigSync.enabled);

    if (dbConfigSync.enabled) {
        dbConfigSync.first = 30;
        if (Io_ConfigGetInt("nxMyDB", "Sync_First", &dbConfigSync.first) && dbConfigSync.first <= 0) {
            LOG_ERROR("Option 'SyncTimer' must be greater than zero.");
            return FALSE;
        }
        dbConfigSync.first = dbConfigSync.first * 1000; // sec to msec

        dbConfigSync.interval = 60;
        if (Io_ConfigGetInt("nxMyDB", "Sync_Interval", &dbConfigSync.interval) && dbConfigSync.interval <= 0) {
            LOG_ERROR("Option 'Sync_Interval' must be greater than zero.");
            return FALSE;
        }
        dbConfigSync.interval = dbConfigSync.interval * 1000; // sec to msec
    }

    //
    // Read server options
    //

    dbConfigServer.host     = ConfigGet("nxMyDB", "Host");
    dbConfigServer.user     = ConfigGet("nxMyDB", "User");
    dbConfigServer.password = ConfigGet("nxMyDB", "Password");
    dbConfigServer.database = ConfigGet("nxMyDB", "Database");
    Io_ConfigGetInt("nxMyDB", "Port", &dbConfigServer.port);
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
    A Windows API error code.

--*/
static DWORD FCALL ConfigFree(VOID)
{
    // Free server options
    if (dbConfigServer.host != NULL) {
        Io_Free(dbConfigServer.host);
    }
    if (dbConfigServer.user != NULL) {
        Io_Free(dbConfigServer.user);
    }
    if (dbConfigServer.password != NULL) {
        Io_Free(dbConfigServer.password);
    }
    if (dbConfigServer.database != NULL) {
        Io_Free(dbConfigServer.database);
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
    ZeroMemory(&dbConfigGlobal, sizeof(DB_CONFIG_GLOBAL));
    ZeroMemory(&dbConfigLock,   sizeof(DB_CONFIG_LOCK));
    ZeroMemory(&dbConfigPool,   sizeof(DB_CONFIG_POOL));
    ZeroMemory(&dbConfigSync,   sizeof(DB_CONFIG_SYNC));
    ZeroMemory(&dbConfigServer, sizeof(DB_CONFIG_SERVER));
    return ERROR_SUCCESS;
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
    while (length > 0 && IS_SPACE(value[length-1])) {
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
    A Windows API error code.

--*/
static DWORD FCALL ConfigUuid(VOID)
{
    CHAR    *format;
    DWORD   result;
    UUID    uuid;

    result = UuidCreate(&uuid);
    switch (result) {
        case RPC_S_OK:
        case RPC_S_UUID_LOCAL_ONLY:
        case RPC_S_UUID_NO_ADDRESS:
            // These are acceptable failures.
            break;
        default:
            LOG_ERROR("Unable to generate UUID (error %lu).", result);
            return FALSE;
    }

    result = UuidToStringA(&uuid, (RPC_CSTR *)&format);
    if (result != RPC_S_OK) {
        LOG_ERROR("Unable to convert UUID (error %lu).", result);
        return result;
    }

    // Copy formatted UUID
    StringCchCopyA(dbConfigLock.owner, ELEMENT_COUNT(dbConfigLock.owner), format);
    dbConfigLock.ownerLength = strlen(dbConfigLock.owner);

    LOG_INFO("Server lock UUID is \"%s\".", format);

    RpcStringFree((RPC_CSTR *)&format);
    return ERROR_SUCCESS;
}


/*++

SyncGetTime

    Retrieves the current server time, as a UNIX timestamp.

Arguments:
    timePtr - Pointer to a variable to receive the server time.

Return Values:
    Windows error code.

--*/
static DWORD SyncGetTime(DB_CONTEXT *db, ULONG *timePtr)
{
    CHAR        *query;
    INT         result;
    MYSQL_BIND  bind[1];
    MYSQL_RES   *metadata;
    MYSQL_STMT  *stmt;

    ASSERT(db != NULL);
    ASSERT(timePtr != NULL);

    stmt = db->stmt[0];

    //
    // Prepare statement and bind parameters
    //

    query = "SELECT UNIX_TIMESTAMP()";

    result = mysql_stmt_prepare(stmt, query, strlen(query));
    if (result != 0) {
        LOG_WARN("Unable to prepare statement: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    metadata = mysql_stmt_result_metadata(stmt);
    if (metadata == NULL) {
        LOG_WARN("Unable to retrieve result metadata: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    result = mysql_stmt_execute(stmt);
    if (result != 0) {
        LOG_ERROR("Unable to execute statement: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    //
    // Bind and fetch results
    //

    DB_CHECK_RESULTS(bind, metadata);
    ZeroMemory(&bind, sizeof(bind));

    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer      = timePtr;
    bind[0].is_unsigned = TRUE;

    result = mysql_stmt_bind_result(stmt, bind);
    if (result != 0) {
        LOG_WARN("Unable to bind results: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    result = mysql_stmt_store_result(stmt);
    if (result != 0) {
        LOG_WARN("Unable to buffer results: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    result = mysql_stmt_fetch(stmt);
    if (result != 0) {
        LOG_WARN("Unable to fetch results: %s", mysql_stmt_error(stmt));
        return DbMapErrorFromStmt(stmt);
    }

    mysql_free_result(metadata);

    return ERROR_SUCCESS;
}

/*++

SyncTimer

    Synchronizes the local user and group cache.

Arguments:
    context - Pointer to the timer context.

    timer   - Pointer to the current TIMER structure.

Return Values:
    Number of milliseconds in which to execute this timer again.

--*/
static DWORD SyncTimer(VOID *context, TIMER *timer)
{
    DB_CONTEXT  *db;
    DWORD       result;
    ULONG       currentTime;

    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(timer);

    TRACE("context=%p timer=%p", context, timer);

    if (DbAcquire(&db)) {
        // Retrieve the current server time
        result = SyncGetTime(db, &currentTime);
        if (result != ERROR_SUCCESS) {
            LOG_ERROR("Unable to retrieve server timestamp (error %lu).", result);

        } else {
            // Update the current time
            dbConfigSync.sync.currUpdate = currentTime;

            // Groups must be updated before users
            DbGroupSync(db, &dbConfigSync.sync);
            DbUserSync(db, &dbConfigSync.sync);

            // Update the previous time
            dbConfigSync.sync.prevUpdate = currentTime;
        }

        DbRelease(db);
    }

    // Execute the timer again
    return dbConfigSync.interval;
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
    This function must be called once by each module entry point. Synchronization
    is not important at this point because ioFTPD performs all module loading and
    initialization in a single thread at start-up.

--*/
BOOL FCALL DbInit(Io_GetProc *getProc)
{
    DWORD result;

    TRACE("refCount=%d", refCount);

    // Only initialize the database pool once
    if (refCount++) {
        TRACE("Already initialized, returning.");
        return TRUE;
    }

    // Initialize configuration structures
    ConfigInit();

    // Initialize procedure table
    result = ProcTableInit(getProc);
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to initialize procedure table (error %lu).", result);
        return FALSE;
    }

    // Initialize logging system
    result = LogInit();
    if (result != ERROR_SUCCESS) {
        TRACE("Unable to initialize logging system (error %lu).", result);
        return FALSE;
    }

    // Load configuration options
    if (!ConfigLoad()) {
        LOG_ERROR("Unable to load configuration.");

        DbFinalize();
        return FALSE;
    }

    // Set log verbosity level
    LogSetLevel((LOG_LEVEL)dbConfigGlobal.logLevel);

    // Generate a UUID for this server
    result = ConfigUuid();
    if (result != ERROR_SUCCESS) {
        LOG_ERROR("Unable to generate UUID (error %lu).", result);

        DbFinalize();
        return FALSE;
    }

    // Create connection pool
    result = PoolCreate(&pool,
        dbConfigPool.minimum, dbConfigPool.average,
        dbConfigPool.maximum, dbConfigPool.timeoutMili,
        ConnectionOpen, ConnectionCheck, ConnectionClose, NULL);

    if (result != ERROR_SUCCESS) {
        LOG_ERROR("Unable to initialize connection pool (error %lu).", result);

        DbFinalize();
        return FALSE;
    }

    LOG_INFO("nxMyDB v%s loaded, using MySQL Client Library v%s.",
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
    TRACE("refCount=%d", refCount);

    // Finalize once the reference count reaches zero
    if (--refCount == 0) {

        // Stop the sync timer
        DbSyncStop();

        // Destroy connection pool
        PoolDestroy(&pool);

        // Free configuration options
        ConfigFree();

        // Stop logging system
        LOG_INFO("nxMyDB v%s unloaded.", STRINGIFY(VERSION));
        LogFinalize();

        ProcTableFinalize();
    }
}

/*++

DbSyncStart

    Starts the database synchronization timer.

Arguments:
    None.

Return Values:
    None.

--*/
VOID FCALL DbSyncStart(VOID)
{
    if (dbConfigSync.enabled) {
        ASSERT(dbConfigSync.timer == NULL);
        dbConfigSync.timer = Io_StartIoTimer(NULL, SyncTimer, NULL, dbConfigSync.first);
    }
}

/*++

DbSyncStop

    Stops the database synchronization timer.

Arguments:
    None.

Return Values:
    None.

--*/
VOID FCALL DbSyncStop(VOID)
{
    if (dbConfigSync.timer != NULL) {
        Io_StopIoTimer(dbConfigSync.timer, FALSE);
        dbConfigSync.timer = NULL;
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
            LOG_INFO("Unknown MySQL result error %lu.", error);
            result = ERROR_INVALID_FUNCTION;
    }

    return result;
}

/*++

DbAcquire

    Acquires a database context from the connection pool.

Arguments:
    dbPtr   - Pointer to a pointer that receives the DB_CONTEXT structure.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
BOOL FCALL DbAcquire(DB_CONTEXT **dbPtr)
{
    ASSERT(dbPtr != NULL);
    TRACE("dbPtr=%p", dbPtr);

    // Acquire a database context
    if (!PoolAcquire(&pool, dbPtr)) {
        LOG_ERROR("Unable to acquire a database context (error %lu).", GetLastError());
        return FALSE;
    }

    return TRUE;
}

/*++

DbRelease

    Releases a database context back into the connection pool.

Arguments:
    db  - Pointer to the DB_CONTEXT structure.

Return Values:
    None.

--*/
VOID FCALL DbRelease(DB_CONTEXT *db)
{
    ASSERT(db != NULL);
    TRACE("db=%p", db);

    // Update last-use time stamp
    GetSystemTimeAsFileTime((FILETIME *)&db->used);

    // Release the database context
    if (!PoolRelease(&pool, db)) {
        LOG_ERROR("Unable to release the database context (error %lu).", GetLastError());
    }
}
