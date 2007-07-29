/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Database Connection

Author:
    neoxed (neoxed@gmail.com) Jun 13, 2006

Abstract:
    Database connection and management functions.

*/

#include "mydb.h"

// Pool resource functions
static POOL_CONSTRUCTOR_PROC ConnectionOpen;
static POOL_VALIDATOR_PROC   ConnectionCheck;
static POOL_DESTRUCTOR_PROC  ConnectionClose;

// MySQL Server information
static char *serverHost;
static char *serverUser;
static char *serverPass;
static char *serverDb;
static int   serverPort;
static BOOL  compression;
static BOOL  sslEnable;
static char *sslCiphers;
static char *sslCertFile;
static char *sslKeyFile;
static char *sslCAFile;
static char *sslCAPath;

// Connection expiration
static UINT64 connCheck;
static UINT64 connExpire;

// Refresh timer
static int refresh;
static TIMER *timer;

// Database connection pool
static POOL *pool;

// Reference count initialization calls
static int refCount = 0;

// Statements to pre-compile
static const char *queries[] = {
    // locking
    "SELECT GET_LOCK(?, ?)"
    ,
    "DO RELEASE_LOCK(?)"
    ,

    // io_groups
    "DELETE FROM io_groups WHERE name=?"
    ,
    "INSERT INTO io_groups (name, updated) VALUES(?, UNIX_TIMESTAMP())"
    ,
    "SELECT description, slots, users, vfsfile FROM io_groups WHERE name=?"
    ,
    "SELECT name FROM io_groups WHERE name=?"
    ,
    "SELECT name, description, slots, users, vfsfile, UNIX_TIMESTAMP() FROM io_groups WHERE updated>=?"
    ,
    "UPDATE io_groups SET description=?, slots=?, users=?, vfsfile=?, updated=UNIX_TIMESTAMP() WHERE name=?"
    ,

    // io_users
    "DELETE FROM io_users WHERE name=?"
    ,
    "INSERT INTO io_users (name, updated) VALUES(?, UNIX_TIMESTAMP())"
    ,
    "SELECT description, flags, home, limits, password, vfsfile, credits, ratio, "
        "alldn, allup, daydn, dayup, monthdn, monthup, wkdn, wkup FROM io_users WHERE name=?"
    ,
    "SELECT name FROM io_users WHERE name=?"
    ,
    "SELECT name, description, flags, home, limits, password, vfsfile, "
        "credits, ratio, alldn, allup, daydn, dayup, monthdn, monthup, wkdn, wkup, "
        "UNIX_TIMESTAMP() FROM io_users WHERE updated>=?"
    ,
    "UPDATE io_users SET description=?, flags=?, home=?, limits=?, password=?, "
        "vfsfile=?, credits=?, ratio=?, alldn=?, allup=?, daydn=?, dayup=?, monthdn=?, "
        "monthup=?, wkdn=?, wkup=?, updated=UNIX_TIMESTAMP() WHERE name=?"
    ,

    // io_useradmins
    "DELETE FROM io_useradmins WHERE uname=?"
    ,
    "INSERT INTO io_useradmins (uname, gname) VALUES(?, ?)"
    ,
    "SELECT gname FROM io_useradmins WHERE uname=?"
    ,

    // io_usergroups
    "DELETE FROM io_usergroups WHERE uname=?"
    ,
    "INSERT INTO io_usergroups (uname, gname) VALUES(?, ?)"
    ,
    "SELECT gname FROM io_usergroups WHERE uname=?"
    ,

    // io_userhosts
    "DELETE FROM io_userhosts WHERE name=?"
    ,
    "INSERT INTO io_userhosts (name, host) VALUES(?, ?)"
    ,
    "SELECT host FROM io_userhosts WHERE name=?"
    ,
};


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
static
BOOL
ConnectionOpen(
    void *opaque,
    void **data
    )
{
    DB_CONTEXT *context;
    DWORD i;
    unsigned long flags;

    Assert(opaque == NULL);
    Assert(data != NULL);
    DebugPrint("ConnectionOpen", "opaque=%p data=%p\n", opaque, data);

    // Allocate database context
    context = Io_Allocate(sizeof(DB_CONTEXT));
    if (context == NULL) {
        DebugPrint("ConnectionOpen", "Unable to allocate memory for database context.\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    ZeroMemory(context, sizeof(DB_CONTEXT));

    // Have MySQL allocate the structure. This is in case the client library is a different
    // version than the header we're compiling with (structures could be different sizes).
    context->handle = mysql_init(NULL);
    if (context->handle == NULL) {
        DebugPrint("ConnectionOpen", "Unable to allocate memory for MySQL handle.\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    // Set client options
    flags = CLIENT_INTERACTIVE;
    if (compression) {
        flags |= CLIENT_COMPRESS;
    }
    if (sslEnable) {
        mysql_ssl_set(context->handle, sslKeyFile, sslCertFile, sslCAFile, sslCAPath, sslCiphers);
    }

    // Open server connection
    if (!mysql_real_connect(context->handle, serverHost, serverUser, serverPass, serverDb, serverPort, NULL, flags)) {
        DebugPrint("ConnectionOpen", "Unable to connect to server: %s\n", mysql_error(context->handle));
        Io_Putlog(LOG_ERROR, "nxMyDB: Unable to connect to server: %s\r\n", mysql_error(context->handle));

        mysql_close(context->handle);
        SetLastError(ERROR_CONNECTION_REFUSED);
        return FALSE;
    }

    // Prepare statements
    Assert(ElementCount(context->stmt) == ElementCount(queries));
    for (i = 0; i < ElementCount(context->stmt); i++) {
        context->stmt[i] = mysql_stmt_init(context->handle);

        if (context->stmt[i] == NULL) {
            DebugPrint("ConnectionOpen", "Unable to allocate memory for statement.\n");
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);

        } else if (mysql_stmt_prepare(context->stmt[i], queries[i], strlen(queries[i])) != 0) {
            DebugPrint("ConnectionOpen", "Unable to prepare statement: %s\n", mysql_stmt_error(context->stmt[i]));
            SetLastError(ERROR_UNIDENTIFIED_ERROR);

        } else {
            continue;
        }

        ConnectionClose(NULL, context);
        return FALSE;
    }

    // Update time stamps
    GetSystemTimeAsFileTime((FILETIME *)&context->created);
    context->used = context->created;

    DebugPrint("ConnectionOpen", "Connected to %s, running MySQL Server v%s.\n",
        mysql_get_host_info(context->handle), mysql_get_server_info(context->handle));

    *data = context;
    return TRUE;
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
static
BOOL
ConnectionCheck(
    void *opaque,
    void *data
    )
{
    DB_CONTEXT *context;
    UINT64 timeCurrent;
    UINT64 timeDelta;

    Assert(opaque == NULL);
    Assert(data != NULL);
    DebugPrint("ConnectionCheck", "opaque=%p data=%p\n", opaque, data);

    context = data;
    GetSystemTimeAsFileTime((FILETIME *)&timeCurrent);

    // Check if the context has exceeded the expiration time
    timeDelta = timeCurrent - context->created;
    if (timeDelta > connExpire) {
        DebugPrint("ConnectionCheck", "Expiring server connection after %I64u seconds.\n", timeDelta/10000000);
        SetLastError(ERROR_CONTEXT_EXPIRED);
        return FALSE;
    }

    // Check if the connection is still alive
    timeDelta = timeCurrent - context->used;
    if (timeDelta > connCheck) {
        DebugPrint("ConnectionCheck", "Connection has not been used in %I64u seconds, pinging it.\n", timeDelta/10000000);

        if (mysql_ping(context->handle) != 0) {
            DebugPrint("ConnectionCheck", "Lost server connection: %s\n", mysql_error(context->handle));
            SetLastError(ERROR_NOT_CONNECTED);
            return FALSE;
        }

        // Update used time stamp
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
static
void
ConnectionClose(
    void *opaque,
    void *data
    )
{
    DB_CONTEXT *context;
    DWORD i;

    Assert(opaque == NULL);
    Assert(data != NULL);
    DebugPrint("ConnectionClose", "opaque=%p data=%p\n", opaque, data);
    context = data;

    // Free prepared statements
    for (i = 0; i < ElementCount(context->stmt); i++) {
        if (context->stmt[i] != NULL) {
            mysql_stmt_close(context->stmt[i]);
        }
    }

    // Close server connection and free context
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

    Assert(array != NULL);
    Assert(variable != NULL);

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

FormatLock

    Format's the lock string.

Arguments:
    lockType     - Type of lock.

    lockName     - Pointer to the lock's name (user/group name).

    buffer       - Pointer to a buffer that recieves the lock string.

    bufferLength - Length of the buffer, in characters.

Return Values:
    None.

--*/
static
void
FormatLock(
    DB_LOCK_TYPE lockType,
    const char *lockName,
    char *buffer,
    size_t bufferLength
    )
{
    char *end;
    char *identifier;

    Assert(lockType == LOCK_TYPE_USER || lockType == LOCK_TYPE_GROUP);
    Assert(lockName != NULL);
    Assert(buffer   != NULL);

    if (lockType == LOCK_TYPE_USER) {
        identifier = ".nxMyDB.user.";
    } else {
        identifier = ".nxMyDB.group.";
    }

    // Format: <database>.nxMyDB.<type>.<name>
    StringCchCopyEx(buffer, bufferLength, serverDb, &end, &bufferLength, 0);
    StringCchCopyEx(end,    bufferLength, identifier, &end, &bufferLength, 0);
    StringCchCopyEx(end,    bufferLength, lockName, &end, &bufferLength, 0);
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
static
DWORD
RefreshTimer(
    void *notUsed,
    TIMER *currTimer
    )
{
    DB_CONTEXT *context;
    UnreferencedParameter(notUsed);
    DebugPrint("RefreshTimer", "currTimer=%p\n", currTimer);

    if (DbAcquire(&context)) {
        // Users rely on groups, so update groups first.
        DbGroupRefresh(context);
        DbUserRefresh(context);

        DbRelease(context);
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
    int poolCheck;
    int poolExpire;
    int poolTimeout;
    DebugPrint("DbInit", "getProc=%p refCount=%i\n", getProc, refCount);

    // Only initialize the database pool once
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

    // Refesh timer
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

    // Start database refresh timer
    if (refresh > 0) {
        timer = Io_StartIoTimer(NULL, RefreshTimer, NULL, refresh);
    } else {
        timer = NULL;
    }

    Io_Putlog(LOG_ERROR, "nxMyDB: v%s loaded, using MySQL Client Library v%s.\r\n",
        Stringify(VERSION), mysql_get_client_info());
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
        Io_Putlog(LOG_ERROR, "nxMyDB: v%s unloaded.\r\n", Stringify(VERSION));

        // Stop refresh timer
        if (timer != NULL) {
            Io_StopIoTimer(timer, FALSE);
        }

        // Destroy connection pool
        PoolDestroy(pool);
        Io_Free(pool);

        ConfigFree();
        ProcTableFinalize();
    }
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
BOOL
DbAcquire(
    DB_CONTEXT **dbContext
    )
{
    DB_CONTEXT *context;

    Assert(dbContext != NULL);
    DebugPrint("DbAcquire", "dbContext=%p\n", dbContext);

    // Acquire a database context
    if (!PoolAcquire(pool, &context)) {
        DebugPrint("DbAcquire", "Unable to acquire a database context (error %lu).\n", GetLastError());
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
void
DbRelease(
    DB_CONTEXT *dbContext
    )
{
    Assert(dbContext != NULL);
    DebugPrint("DbRelease", "dbContext=%p\n", dbContext);

    // Update used time stamp
    GetSystemTimeAsFileTime((FILETIME *)&dbContext->used);

    // Release the database context
    if (!PoolRelease(pool, dbContext)) {
        DebugPrint("DbRelease", "Unable to release the database context (error %lu).\n", GetLastError());
    }
}

/*++

DbLock

    Attempts to lock the specified user or group.

Arguments:
    dbContext   - Pointer to the DB_CONTEXT structure.

    lockType    - Type of lock.

    lockName    - Pointer to the lock's name (user/group name).

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
BOOL
DbLock(
    DB_CONTEXT *dbContext,
    DB_LOCK_TYPE lockType,
    const char *lockName
    )
{
    char lock[128];

    Assert(dbContext != NULL);
    Assert(lockName != NULL);
    DebugPrint("DbLock", "dbContext=%p lockType=%d lockName=%s\n", dbContext, lockType, lockName);

    FormatLock(lockType, lockName, lock, ElementCount(lock));

    // TODO

    return TRUE;
}

/*++

DbUnlock

    Unlocks the specified user or group.

Arguments:
    dbContext   - Pointer to the DB_CONTEXT structure.

    lockType    - Type of lock.

    lockName    - Pointer to the lock's name (user/group name).

Return Values:
    None.

--*/
void
DbUnlock(
    DB_CONTEXT *dbContext,
    DB_LOCK_TYPE lockType,
    const char *lockName
    )
{
    char lock[128];

    Assert(dbContext != NULL);
    Assert(lockName != NULL);
    DebugPrint("DbUnlock", "dbContext=%p lockType=%d lockName=%s\n", dbContext, lockType, lockName);

    FormatLock(lockType, lockName, lock, ElementCount(lock));

    // TODO
}
