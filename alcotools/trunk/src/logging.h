/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Logging

Author:
    neoxed (neoxed@gmail.com) Jul 17, 2005

Abstract:
    Logging function prototypes and macros.

--*/

#ifndef _LOGGING_H_
#define _LOGGING_H_

//
// Message levels, ordered in increasing severity.
//

enum {
    LOG_LEVEL_FATAL = 1,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_VERBOSE
};


//
// Redefine logging macros for debug and release builds.
//

#undef ERROR
#undef FATAL
#undef VERBOSE
#undef WARNING

#if (LOG_LEVEL >= LOG_LEVEL_FATAL)
#   define FATAL(format, ...)   LogFormat(LOG_LEVEL_FATAL, format, __VA_ARGS__)
#else
#   define FATAL(format, ...)   ((void)0)
#endif

#if (LOG_LEVEL >= LOG_LEVEL_ERROR)
#   define ERROR(format, ...)   LogFormat(LOG_LEVEL_ERROR, format, __VA_ARGS__)
#else
#   define ERROR(format, ...)   ((void)0)
#endif

#if (LOG_LEVEL >= LOG_LEVEL_WARNING)
#   define WARNING(format, ...) LogFormat(LOG_LEVEL_WARNING, format, __VA_ARGS__)
#else
#   define WARNING(format, ...) ((void)0)
#endif

#if (LOG_LEVEL >= LOG_LEVEL_VERBOSE)
#   define VERBOSE(format, ...) LogFormat(LOG_LEVEL_VERBOSE, format, __VA_ARGS__)
#else
#   define VERBOSE(format, ...) ((void)0)
#endif


//
// Logging functions
//

#if (LOG_LEVEL > 0)

apr_status_t
LogInit(
    apr_pool_t *pool
    );

void
LogFinalize(
    void
    );

void
LogFormat(
    int level,
    const char *format,
    ...
    );

void
LogFormatV(
    int level,
    const char *message,
    va_list argList
    );

#endif // LOG_LEVEL

#endif // _LOGGING_H_
