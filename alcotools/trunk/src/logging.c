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

static apr_file_t *handle;      // Handle to the log file
static apr_pool_t *msgPool;     // Sub-pool used for formatting log messages
static apr_uint32_t maxLevel;   // Maximum log verbosity level


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
    maxLevel = LOG_LEVEL_OFF;

    if (ConfigGetInt(SectionGeneral, GeneralLogLevel, &maxLevel) != APR_SUCCESS || !maxLevel) {
        return APR_SUCCESS;
    }

    // Create a sub-pool for log message allocations
    status = apr_pool_create(&msgPool, pool);
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

Return Values:
    None.

Remarks:
    This function could be called before the logging subsystem is initialized.

--*/
void
LogFormat(
    apr_uint32_t level,
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
    apr_uint32_t level,
    const char *format,
    va_list argList
    )
{
    apr_time_exp_t now;
    char *message;

    ASSERT(format != NULL);

    if (level <= maxLevel && handle != NULL) {
        // Write local time
        apr_time_exp_lt(&now, apr_time_now());
        apr_file_printf(handle, "%04d-%02d-%02d %02d:%02d:%02d - ",
            now.tm_year+1900, now.tm_mon, now.tm_mday,
            now.tm_hour, now.tm_min, now.tm_sec);

        // Format and write log message
        message = apr_pvsprintf(msgPool, format, argList);
        if (message == NULL) {
            apr_file_puts("Unable to format message." APR_EOL_STR, handle);
        } else {
            apr_file_puts(message, handle);
        }
        apr_file_flush(handle);

        // Clear memory allocated when formatting the message
        apr_pool_clear(msgPool);
    }
}

#endif // LOG_LEVEL
