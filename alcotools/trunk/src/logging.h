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
    LOG_LEVEL_OFF = 0,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_VERBOSE,
    LOG_LEVEL_DEBUG
};


//
// Redefine logging macros for debug and release builds.
//

#undef LOG_ERROR
#undef LOG_WARNING
#undef LOG_VERBOSE
#undef LOG_DEBUG

#if (LOG_LEVEL >= LOG_LEVEL_ERROR)
#   define LOG_ERROR(format, ...)   LogFormat(LOG_LEVEL_ERROR, format, __VA_ARGS__)
#else
#   define LOG_ERROR(format, ...)   ((void)0)
#endif

#if (LOG_LEVEL >= LOG_LEVEL_WARNING)
#   define LOG_WARNING(format, ...) LogFormat(LOG_LEVEL_WARNING, format, __VA_ARGS__)
#else
#   define LOG_WARNING(format, ...) ((void)0)
#endif

#if (LOG_LEVEL >= LOG_LEVEL_VERBOSE)
#   define LOG_VERBOSE(format, ...) LogFormat(LOG_LEVEL_VERBOSE, format, __VA_ARGS__)
#else
#   define LOG_VERBOSE(format, ...) ((void)0)
#endif

#if (LOG_LEVEL >= LOG_LEVEL_FATAL)
#   define LOG_DEBUG(format, ...)   LogFormat(LOG_LEVEL_DEBUG, format, __VA_ARGS__)
#else
#   define LOG_DEBUG(format, ...)   ((void)0)
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
