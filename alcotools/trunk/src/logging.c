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

#define _CRT_SECURE_NO_DEPRECATE
#include "alcoholicz.h"

#if (LOG_LEVEL > 0)

static FILE *logHandle = NULL;

// Default log file and verbosity level.
static tchar_t  *logFile    = TEXT("AlcoTools.log");
static uint32_t maxLogLevel = LOG_LEVEL_VERBOSE;


/*++

LogInit

    Initialise the logging subsystem.

Arguments:
    None.

Return Value:
    If the function succeeds, the return value is nonzero. If the function
    fails, the return value is zero. To get extended error information, call
    GetSystemErrorMessage.

--*/
bool_t
LogInit(
    void
    )
{
    ASSERT(logHandle == NULL);

    // Return if the log file is already option or if logging is disabled.
    if (logHandle != NULL || ConfigGetInt(SectionGeneral, GeneralLogLevel,
            &maxLogLevel) != ALCOHOL_OK || !maxLogLevel) {
        return TRUE;
    }

    logHandle = t_fopen(logFile, TEXT("a"));
    if (logHandle != NULL) {
        t_fputc(TEXT('\n'), logHandle);
        return TRUE;
    }
    return FALSE;
}

/*++

LogFinalise

    Finalise the logging subsystem.

Arguments:
    None.

Return Value:
    None.

--*/
void
LogFinalise(
    void
    )
{
    if (logHandle != NULL) {
        fclose(logHandle);
        logHandle = NULL;
    }
}

/*++

LogFormat

    Adds an entry to the log file.

Arguments:
    level   - Log severity level.

    format  - Pointer to a buffer containing a printf-style format string.

    ...     - Arguments to insert into 'format'.

Remarks:
    This function could be called before the logging subsystem is initialised.

--*/
void
LogFormatA(
    uint32_t level,
    const char *format,
    ...
    )
{
    va_list argList;
    va_start(argList, format);
    LogFormatVA(level, format, argList);
    va_end(argList);
}

#ifdef UNICODE
void
LogFormatW(
    uint32_t level,
    const wchar_t *format,
    ...
    )
{
    va_list argList;
    va_start(argList, format);
    LogFormatVW(level, format, argList);
    va_end(argList);
}
#endif // UNICODE

/*++

LogFormatV

    Adds an entry to the log file.

Arguments:
    level   - Log severity level.

    format  - Pointer to a buffer containing a printf-style format string.

    argList - Argument list to insert into 'format'.

Return Value:
    None.

Remarks:
    This function could be called before the logging subsystem is initialised.

--*/
void
LogFormatVA(
    uint32_t level,
    const char *format,
    va_list argList
    )
{
    ASSERT(format != NULL);

    if (level <= maxLogLevel && logHandle != NULL) {
#ifdef WINDOWS
        SYSTEMTIME now;
        GetSystemTime(&now);

        fprintf(logHandle, "%04d-%02d-%02d %02d:%02d:%02d - ",
            now.wYear, now.wMonth, now.wDay,
            now.wHour, now.wMinute, now.wSecond);
#else
        time_t timer;
        struct tm *now;
        time(&timer);
        now = localtime(&timer);

        fprintf(logHandle, "%04d-%02d-%02d %02d:%02d:%02d - ",
            now->tm_year+1900, now->tm_mon, now->tm_mday,
            now->tm_hour, now->tm_min, now->tm_sec);
#endif // WINDOWS

        vfprintf(logHandle, format, argList);
        fflush(logHandle);
    }
}

#ifdef UNICODE
void
LogFormatVW(
    uint32_t level,
    const wchar_t *format,
    va_list argList
    )
{
    ASSERT(format != NULL);

    if (level <= maxLogLevel && logHandle != NULL) {
#ifdef WINDOWS
        SYSTEMTIME now;
        GetSystemTime(&now);

        fprintf(logHandle, "%04d-%02d-%02d %02d:%02d:%02d - ",
            now.wYear, now.wMonth, now.wDay,
            now.wHour, now.wMinute, now.wSecond);
#else
        time_t timer;
        struct tm *now;
        time(&timer);
        now = localtime(&timer);

        fprintf(logHandle, "%04d-%02d-%02d %02d:%02d:%02d - ",
            now->tm_year+1900, now->tm_mon, now->tm_mday,
            now->tm_hour, now->tm_min, now->tm_sec);
#endif // WINDOWS

        vfwprintf(logHandle, format, argList);
        fflush(logHandle);
    }
}
#endif // UNICODE

#endif // LOG_LEVEL
