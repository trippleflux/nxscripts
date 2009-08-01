/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2009 neoxed

Module Name:
    Configuration Reader

Author:
    neoxed (neoxed@gmail.com) Jun 30, 2009

Abstract:
    Configuration reading functions.

*/

#include <base.h>
#include <config.h>
#include <logging.h>

//
// Global configuration variables
//

DB_CONFIG_GLOBAL  dbConfigGlobal;
DB_CONFIG_LOCK    dbConfigLock;
DB_CONFIG_POOL    dbConfigPool;
DB_CONFIG_SYNC    dbConfigSync;

DB_CONFIG_SERVER  *dbConfigServers;
DWORD             dbConfigServerCount;

//
// Function declarations
//

static CHAR *FCALL GetString(CHAR *array, CHAR *variable);
static DWORD FCALL SetUuid(VOID);
static VOID  FCALL ZeroConfig(VOID);

static DWORD FCALL LoadGlobal(VOID);
static DWORD FCALL LoadServer(CHAR *array, DB_CONFIG_SERVER *server);
static DWORD FCALL LoadServers(VOID);
static DWORD FCALL FreeServer(DB_CONFIG_SERVER *server);
static DWORD FCALL FreeServers(VOID);


/*++

GetString

    Retrieves configuration options, also removing comments and whitespace.

Arguments:
    array    - Option array name.

    variable - Option variable name.

Return Values:
    If the function succeeds, the return value is a pointer to the configuration value.

    If the function fails, the return value is null.

--*/
static CHAR *FCALL GetString(CHAR *array, CHAR *variable)
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

SetUuid

    Generates a UUID used for identifying the server.

Arguments:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

Return Values:
    A Windows API error code.

--*/
static DWORD FCALL SetUuid(VOID)
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
            return result;
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

ZeroConfig

    Zeros all configuration structures.

Arguments:
    None.

Return Values:
    None.

--*/
static VOID FCALL ZeroConfig(VOID)
{
    dbConfigServers     = NULL;
    dbConfigServerCount = 0;

    ZeroMemory(&dbConfigGlobal, sizeof(DB_CONFIG_GLOBAL));
    ZeroMemory(&dbConfigLock,   sizeof(DB_CONFIG_LOCK));
    ZeroMemory(&dbConfigPool,   sizeof(DB_CONFIG_POOL));
    ZeroMemory(&dbConfigSync,   sizeof(DB_CONFIG_SYNC));
}


/*++

LoadGlobal

    Loads global configuration options.

Arguments:
    None.

Return Values:
    A Windows API error code.

--*/
static DWORD FCALL LoadGlobal(VOID)
{
    INT value;

    //
    // Read logging options
    //

    value = (INT)LOG_LEVEL_ERROR;
    Io_ConfigGetInt("nxMyDB", "Log_Level", &value);
    dbConfigGlobal.logLevel = (LOG_LEVEL)value;

    //
    // Read lock options
    //

    dbConfigLock.expire = 60;
    if (Io_ConfigGetInt("nxMyDB", "Lock_Expire", &dbConfigLock.expire) && dbConfigLock.expire <= 0) {
        LOG_ERROR("Configuration option 'Lock_Expire' must be greater than zero.");
        return ERROR_INVALID_PARAMETER;
    }

    dbConfigLock.timeout = 5;
    if (Io_ConfigGetInt("nxMyDB", "Lock_Timeout", &dbConfigLock.timeout) && dbConfigLock.timeout <= 0) {
        LOG_ERROR("Configuration option 'Lock_Timeout' must be greater than zero.");
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Read pool options
    //

    dbConfigPool.minimum = 1;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Minimum", &dbConfigPool.minimum) && dbConfigPool.minimum <= 0) {
        LOG_ERROR("Configuration option 'Pool_Minimum' must be greater than zero.");
        return ERROR_INVALID_PARAMETER;
    }

    dbConfigPool.average = dbConfigPool.minimum + 1;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Average", &dbConfigPool.average) && dbConfigPool.average < dbConfigPool.minimum) {
        LOG_ERROR("Configuration option 'Pool_Average' must be greater than or equal to 'Pool_Minimum'.");
        return ERROR_INVALID_PARAMETER;
    }

    dbConfigPool.maximum = dbConfigPool.average * 2;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Maximum", &dbConfigPool.maximum) && dbConfigPool.maximum < dbConfigPool.average) {
        LOG_ERROR("Configuration option 'Pool_Maximum' must be greater than or equal to 'Pool_Average'.");
        return ERROR_INVALID_PARAMETER;
    }

    dbConfigPool.timeout = 5;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Timeout", &dbConfigPool.timeout) && dbConfigPool.timeout <= 0) {
        LOG_ERROR("Configuration option 'Pool_Timeout' must be greater than zero.");
        return ERROR_INVALID_PARAMETER;
    }
    dbConfigPool.timeoutMili = dbConfigPool.timeout * 1000; // sec to msec

    dbConfigPool.expire = 3600;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Expire", &dbConfigPool.expire) && dbConfigPool.expire <= 0) {
        LOG_ERROR("Configuration option 'Pool_Expire' must be greater than zero.");
        return ERROR_INVALID_PARAMETER;
    }
    dbConfigPool.expireNano = UInt32x32To64(dbConfigPool.expire, 10000000); // sec to 100nsec

    dbConfigPool.check = 60;
    if (Io_ConfigGetInt("nxMyDB", "Pool_Check", &dbConfigPool.check) &&
            (dbConfigPool.check <= 0 || dbConfigPool.check >= dbConfigPool.expire)) {
        LOG_ERROR("Configuration option 'Pool_Check' must be greater than zero and less than 'Pool_Expire'.");
        return ERROR_INVALID_PARAMETER;
    }
    dbConfigPool.checkNano = UInt32x32To64(dbConfigPool.check, 10000000); // sec to 100nsec

    //
    // Read sync options
    //

    Io_ConfigGetBool("nxMyDB", "Sync", &dbConfigSync.enabled);

    if (dbConfigSync.enabled) {
        dbConfigSync.first = 30;
        if (Io_ConfigGetInt("nxMyDB", "Sync_First", &dbConfigSync.first) && dbConfigSync.first <= 0) {
            LOG_ERROR("Configuration option 'SyncTimer' must be greater than zero.");
            return ERROR_INVALID_PARAMETER;
        }
        dbConfigSync.first = dbConfigSync.first * 1000; // sec to msec

        dbConfigSync.interval = 60;
        if (Io_ConfigGetInt("nxMyDB", "Sync_Interval", &dbConfigSync.interval) && dbConfigSync.interval <= 0) {
            LOG_ERROR("Configuration option 'Sync_Interval' must be greater than zero.");
            return ERROR_INVALID_PARAMETER;
        }
        dbConfigSync.interval = dbConfigSync.interval * 1000; // sec to msec

        dbConfigSync.purge = dbConfigSync.interval * 100;
        if (Io_ConfigGetInt("nxMyDB", "Sync_Purge", &dbConfigSync.purge) && dbConfigSync.purge <= dbConfigSync.interval) {
            LOG_ERROR("Configuration option 'Sync_Purge' must be greater than 'Sync_Interval'.");
            return ERROR_INVALID_PARAMETER;
        }
    }

    return ERROR_SUCCESS;
}

/*++

LoadServer

    Load a server configuration.

Arguments:
    array   - Option array name.

    server  - Pointer to a DB_CONFIG_SERVER structure.

Return Values:
    A Windows API error code.

Remarks:
    This function always succeeds.

--*/
static DWORD FCALL LoadServer(CHAR *array, DB_CONFIG_SERVER *server)
{
    ASSERT(array != NULL);
    ASSERT(server != NULL);

    // Host options
    server->host     = GetString(array, "Host");
    server->user     = GetString(array, "User");
    server->password = GetString(array, "Password");
    server->database = GetString(array, "Database");
    Io_ConfigGetInt(array, "Port", &server->port);

    // Connection options
    Io_ConfigGetBool(array, "Compression", &server->compression);
    Io_ConfigGetBool(array, "SSL_Enable", &server->sslEnable);
    server->sslCiphers  = GetString(array, "SSL_Ciphers");
    server->sslCertFile = GetString(array, "SSL_Cert_File");
    server->sslKeyFile  = GetString(array, "SSL_Key_File");
    server->sslCAFile   = GetString(array, "SSL_CA_File");
    server->sslCAPath   = GetString(array, "SSL_CA_Path");

    return ERROR_SUCCESS;
}

/*++

LoadServers

    Loads all server configurations.

Arguments:
    None.

Return Values:
    A Windows API error code.

--*/
static DWORD FCALL LoadServers(VOID)
{
    CHAR        *name;
    CHAR        *servers;
    DWORD       count;
    DWORD       i;
    DWORD       result;
    IO_STRING   serverList;

    // Retrieve the list of server array names
    servers = GetString("nxMyDB", "Servers");
    if (servers == NULL) {
        LOG_ERROR("Configuration option 'Servers' must be defined.");
        return ERROR_INVALID_PARAMETER;
    }

    if (Io_SplitString(servers, &serverList) != 0) {
        LOG_ERROR("Unable to allocate memory for server list.");
        result = ERROR_NOT_ENOUGH_MEMORY;
    } else {
        // Retrieve the number of items in the server list
        count = GetStringItems(&serverList);
        if (count < 1) {
            LOG_ERROR("Configuration option 'Servers' must contain at least one server.");
            result = ERROR_INVALID_PARAMETER;

        } else {
            // Allocate an array of server structures
            dbConfigServers = MemAllocate(sizeof(DB_CONFIG_SERVER) * count);
            if (dbConfigServers == NULL) {
                LOG_ERROR("Unable to allocate memory for server array.");
                result = ERROR_NOT_ENOUGH_MEMORY;

            } else {
                // Load the server configuration for each array
                for (i = 0; i < count; i++) {
                    name = Io_GetStringIndexStatic(&serverList, i);

                    // The success of LoadServer() does not matter
                    LoadServer(name, &dbConfigServers[i]);
                }

                // Update global count
                dbConfigServerCount = count;

                // Succesfully loaded at least one server configuration
                result = ERROR_SUCCESS;
            }
        }
        Io_FreeString(&serverList);
    }

    Io_Free(servers);
    return result;
}

/*++

FreeServer

    Frees memory allocated for a server configuration structure.

Arguments:
    server  - Pointer to a DB_CONFIG_SERVER structure.

Return Values:
    A Windows API error code.

Remarks:
    This function always succeeds.

--*/
static DWORD FCALL FreeServer(DB_CONFIG_SERVER *server)
{
    ASSERT(server != NULL);

    // Free server options
    if (server->host != NULL) {
        Io_Free(server->host);
    }
    if (server->user != NULL) {
        Io_Free(server->user);
    }
    if (server->password != NULL) {
        Io_Free(server->password);
    }
    if (server->database != NULL) {
        Io_Free(server->database);
    }

    // Free SSL options
    if (server->sslCiphers != NULL) {
        Io_Free(server->sslCiphers);
    }
    if (server->sslCertFile != NULL) {
        Io_Free(server->sslCertFile);
    }
    if (server->sslKeyFile != NULL) {
        Io_Free(server->sslKeyFile);
    }
    if (server->sslCAFile != NULL) {
        Io_Free(server->sslCAFile);
    }
    if (server->sslCAPath != NULL) {
        Io_Free(server->sslCAPath);
    }

    // Clear structure in case a pointer is mistakenly re-used
    ZeroMemory(server, sizeof(DB_CONFIG_SERVER));

    return ERROR_SUCCESS;
}

/*++

FreeServers

    Frees all server configuration structures.

Arguments:
    None.

Return Values:
    A Windows API error code.

Remarks:
    This function always succeeds.

--*/
static DWORD FCALL FreeServers(VOID)
{
    DWORD i;

    if (dbConfigServers != NULL) {
        // There should always be at least one server
        ASSERT(dbConfigServerCount > 0);

        // Free all server options and the server array
        for (i = 0; i < dbConfigServerCount; i++) {
            FreeServer(&dbConfigServers[i]);
        }
        Io_Free(dbConfigServers);

        dbConfigServers     = NULL;
        dbConfigServerCount = 0;
    }

    return ERROR_SUCCESS;
}


/*++

ConfigInit

    Initializes configuration structures.

Arguments:
    None.

Return Values:
    A Windows API error code.

--*/
DWORD FCALL ConfigInit(VOID)
{
    // Clear configuration structures
    ZeroConfig();

    return ERROR_SUCCESS;
}

/*++

ConfigLoad

    Loads configuration values.

Arguments:
    None.

Return Values:
    A Windows API error code.

--*/
DWORD FCALL ConfigLoad(VOID)
{
    DWORD result;

    //
    // Each of these following functions will log
    // the appropriate message if they fail.
    //

    // Load global configuration options
    result = LoadGlobal();
    if (result != ERROR_SUCCESS) {
        return result;
    }

    // Load server configurations
    result = LoadServers();
    if (result != ERROR_SUCCESS) {
        return result;
    }

    // Set the lock UUID for this server
    result = SetUuid();
    if (result != ERROR_SUCCESS) {
        return result;
    }

    // There should always be at least one server
    ASSERT(dbConfigServers != NULL);
    ASSERT(dbConfigServerCount > 0);

    return ERROR_SUCCESS;
}

/*++

ConfigFinalize

    Finalizes configuration structures.

Arguments:
    None.

Return Values:
    A Windows API error code.

--*/
DWORD FCALL ConfigFinalize(VOID)
{
    // Free all server configuration structures
    FreeServers();

    // Clear configuration structures
    ZeroConfig();

    return ERROR_SUCCESS;
}
