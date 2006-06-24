/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Logging

Author:
    neoxed (neoxed@gmail.com) Jul 17, 2005

Abstract:
    This module implements a logging interface to redirect information
    to a file or a standard output device (stdout or stderr).

--*/

#include "alcoholicz.h"

#if (LOG_LEVEL > 0)

static apr_file_t *handle;
static apr_pool_t *subPool;
static int maxLevel;


/*++

LogInit

    Initialize the logging subsystem.

Arguments:
    pool    - Main application pool, to create sub-pools from.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
LogInit(
    apr_pool_t *pool
    )
{
    apr_status_t status;

    // Initialize static variables
    handle = NULL;
    maxLevel = 0;

    if (ConfigGetInt(SectionGeneral, GeneralLogLevel, &maxLevel) != APR_SUCCESS || !maxLevel) {
        return APR_SUCCESS;
    }

    // Create a sub-pool for log message allocations
    status = apr_pool_create(&subPool, pool);
    if (status != APR_SUCCESS) {
        return status;
    }

    // Open log file for writing
    return apr_file_open(&handle, LOG_FILE, APR_FOPEN_WRITE|
        APR_FOPEN_CREATE|APR_FOPEN_APPEND, APR_OS_DEFAULT, pool);
}

/*++

LogFormat

    Adds an entry to the log file.

Arguments:
    level   - Log severity level.

    format  - Pointer to a buffer containing a printf-style format string.

    ...     - Arguments to insert into 'format'.

Remarks:
    This function could be called before the logging subsystem is initialized.

--*/
void
LogFormat(
    int level,
    const char *format,
    ...
    )
{
    va_list argList;
    va_start(argList, format);
    LogFormatV(level, format, argList);
    va_end(argList);
}

/*++

LogFormatV

    Adds an entry to the log file.

Arguments:
    level   - Log severity level.

    format  - Pointer to a buffer containing a printf-style format string.

    argList - Argument list to insert into 'format'.

Return Values:
    None.

Remarks:
    This function could be called before the logging subsystem is initialized.

--*/
void
LogFormatV(
    int level,
    const char *format,
    va_list argList
    )
{
    ASSERT(format != NULL);

    if (level <= maxLevel && handle != NULL) {
        apr_time_exp_t now;

        // Write local time
        apr_time_exp_lt(&now, apr_time_now());
        apr_file_printf(handle, "%04d-%02d-%02d %02d:%02d:%02d - ",
            now.tm_year+1900, now.tm_mon, now.tm_mday,
            now.tm_hour, now.tm_min, now.tm_sec);

        //TODO: fix
        //vfprintf(handle, format, argList);

        apr_file_flush(handle);
    }
}

#endif // LOG_LEVEL
