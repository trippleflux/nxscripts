/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    Logging

Author:
    neoxed (neoxed@gmail.com) Sep 30, 2007

Abstract:
    Logging interface.

*/

#include <base.h>
#include <logging.h>

#if (LOG_OPTION_BACKEND == LOG_BACKEND_DEBUG)
#   define BACKEND_INIT     LogDebuggerInit
#   define BACKEND_FINAL    LogDebuggerFinalize
#   define BACKEND_FORMAT   LogDebuggerFormatV
#   define BACKEND_TRACE    LogDebuggerTraceV
#elif (LOG_OPTION_BACKEND == LOG_BACKEND_FILE)
#   define BACKEND_INIT     LogFileInit
#   define BACKEND_FINAL    LogFileFinalize
#   define BACKEND_FORMAT   LogFileFormatV
#   define BACKEND_TRACE    LogFileTraceV
#else
#   error Unknown logging backend.
#endif

static LOG_LEVEL logLevel;


DWORD SCALL LogInit(VOID)
{
    // Default to the error log level
    logLevel = 10;

    return BACKEND_INIT();
}

DWORD SCALL LogFinalize(VOID)
{
    return BACKEND_FINAL();
}

DWORD SCALL LogSetLevel(LOG_LEVEL level)
{
    logLevel = level;
    return ERROR_SUCCESS;
}

VOID CCALL LogFormat(LOG_LEVEL level, const CHAR *format, ...)
{
    va_list argList;

    va_start(argList, format);
    LogFormatV(level, format, argList);
    va_end(argList);
}

VOID SCALL LogFormatV(LOG_LEVEL level, const CHAR *format, va_list argList)
{
    if (level <= logLevel) {
        BACKEND_FORMAT(format, argList);
    }
}

VOID CCALL LogTrace(const CHAR *file, const CHAR *func, INT line, LOG_LEVEL level, const CHAR *format, ...)
{
    va_list argList;

    va_start(argList, format);
    LogTraceV(file, func, line, level, format, argList);
    va_end(argList);
}

VOID SCALL LogTraceV(const CHAR *file, const CHAR *func, INT line, LOG_LEVEL level, const CHAR *format, va_list argList)
{
    if (level <= logLevel) {
        BACKEND_TRACE(file, func, line, format, argList);
    }
}
