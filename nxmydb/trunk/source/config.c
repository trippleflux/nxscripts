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
// Configuration variables
//

DB_CONFIG_GLOBAL  dbConfigGlobal;
DB_CONFIG_LOCK    dbConfigLock;
DB_CONFIG_POOL    dbConfigPool;
DB_CONFIG_SERVER  dbConfigServer;
DB_CONFIG_SYNC    dbConfigSync;

//
// Function declarations
//

static CHAR *FCALL GetString(CHAR *array, CHAR *variable);
static VOID  FCALL ZeroConfig(VOID);

static BOOL  FCALL LoadGlobal(VOID);
static BOOL  FCALL LoadServer(CHAR *array);
static DWORD FCALL FreeServer(VOID);


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

LoadGlobal

    Loads global configuration options.

Arguments:
    None.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
static BOOL FCALL LoadGlobal(VOID)
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

        dbConfigSync.purge = dbConfigSync.interval * 100;
        if (Io_ConfigGetInt("nxMyDB", "Sync_Purge", &dbConfigSync.purge) && dbConfigSync.purge <= dbConfigSync.interval) {
            LOG_ERROR("Option 'Sync_Purge' must be greater than 'Sync_Interval'.");
            return FALSE;
        }
    }

    return TRUE;
}

/*++

LoadServer

    Loads server configuration options.

Arguments:
    None.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
static BOOL FCALL LoadServer(CHAR *array)
{
    ASSERT(array != NULL);

    //
    // Read server options
    //

    dbConfigServer.host     = GetString(array, "Host");
    dbConfigServer.user     = GetString(array, "User");
    dbConfigServer.password = GetString(array, "Password");
    dbConfigServer.database = GetString(array, "Database");
    Io_ConfigGetInt(array, "Port", &dbConfigServer.port);
    Io_ConfigGetBool(array, "Compression", &dbConfigServer.compression);
    Io_ConfigGetBool(array, "SSL_Enable", &dbConfigServer.sslEnable);
    dbConfigServer.sslCiphers  = GetString(array, "SSL_Ciphers");
    dbConfigServer.sslCertFile = GetString(array, "SSL_Cert_File");
    dbConfigServer.sslKeyFile  = GetString(array, "SSL_Key_File");
    dbConfigServer.sslCAFile   = GetString(array, "SSL_CA_File");
    dbConfigServer.sslCAPath   = GetString(array, "SSL_CA_Path");

    return TRUE;
}

/*++

FreeServer

    Frees memory allocated for configuration options.

Arguments:
    None.

Return Values:
    A Windows API error code.

--*/
static DWORD FCALL FreeServer(VOID)
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
    ZeroMemory(&dbConfigGlobal, sizeof(DB_CONFIG_GLOBAL));
    ZeroMemory(&dbConfigLock,   sizeof(DB_CONFIG_LOCK));
    ZeroMemory(&dbConfigPool,   sizeof(DB_CONFIG_POOL));
    ZeroMemory(&dbConfigServer, sizeof(DB_CONFIG_SERVER));
    ZeroMemory(&dbConfigSync,   sizeof(DB_CONFIG_SYNC));
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
    LoadGlobal();

    LoadServer("nxMyDB");

    return ERROR_SUCCESS;
}

/*++

ConfigSetUuid

    Generates a UUID used for identifying the server.

Arguments:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

Return Values:
    A Windows API error code.

--*/
DWORD FCALL ConfigSetUuid(VOID)
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

ConfigFinalize

    Finalizes configuration structures.

Arguments:
    None.

Return Values:
    A Windows API error code.

--*/
DWORD FCALL ConfigFinalize(VOID)
{
    // Free server variables
    FreeServer();

    // Clear configuration structures
    ZeroConfig();

    return ERROR_SUCCESS;
}
