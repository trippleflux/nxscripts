/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Unix Platform

Author:
    neoxed (neoxed@gmail.com) Jul 17, 2005

Abstract:
    BSD/Linux/UNIX specific includes and definitions.

--*/

#ifndef _PLATUNIX_H_
#define _PLATUNIX_H_

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef HAVE_MEMORY_H
#   include <memory.h>
#endif
#ifdef HAVE_STRINGS_H
#   include <strings.h>
#endif
#ifdef HAVE_TIME_H
#   include <time.h>
#endif
#ifdef HAVE_UNISTD_H
#   include <unistd.h>
#endif
#ifdef HAVE_SYS_TIME_H
#   include <sys/time.h>
#endif

#ifdef HAVE_LIMITS_H
#   include <limits.h>
#endif
#ifdef HAVE_SYSLIMITS_H
#   include <syslimits.h>
#endif
#ifdef HAVE_SYS_LIMITS_H
#   include <sys/limits.h>
#endif

#ifdef HAVE_DIRENT_H
#   include <dirent.h>
#   define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#   define dirent direct
#   define NAMLEN(dirent) (dirent)->d_namlen
#   ifdef HAVE_SYS_NDIR_H
#       include <sys/ndir.h>
#   endif
#   ifdef HAVE_SYS_DIR_H
#       include <sys/dir.h>
#   endif
#   ifdef HAVE_NDIR_H
#       include <ndir.h>
#   endif
#endif // HAVE_DIRENT_H


//
// Performance Counter
//

typedef struct {
    struct timeval start;
    struct timeval stop;
} PERF_COUNTER;

inline void
PerfCounterStart(
    PERF_COUNTER *counter
    )
{
    gettimeofday(&counter->start);
    counter->stop.tv_sec = 0;
    counter->stop.tv_usec = 0;
}

inline void
PerfCounterStop(
    PERF_COUNTER *counter
    )
{
    gettimeofday(&counter->stop);
}

inline double
PerfCounterDiff(
    PERF_COUNTER *counter
    )
{
    return (double)(counter->stop.tv_usec - counter->start.tv_usec);
}

#endif // _PLATUNIX_H_
