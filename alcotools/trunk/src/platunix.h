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

#ifdef HAVE_INTTYPES_H
#   include <inttypes.h>
#endif
#ifdef HAVE_MEMORY_H
#   include <memory.h>
#endif
#ifdef HAVE_STDINT_H
#   include <stdint.h>
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
#ifdef HAVE_WCHAR_H
#   include <wchar.h>
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
// Data types and related definitions.
//

#define FALSE           0
#define TRUE            1
#define PATH_LENGTH     PATH_MAX

typedef unsigned char   bool_t;
typedef unsigned char   byte_t;

#ifndef HAVE_WCHAR_H
typedef unsigned short  wchar_t;
#endif

//
// glFTPD does not support Unicode, so there's no point
// in trying to internationalise this application for *nix.
//
#undef UNICODE
typedef char            tchar_t;


//
// Memory Functions
//

#ifndef HAVE_MEMCMP
#   define memcmp(a,b,n) bcmp((a),(b),(n))
#endif

#ifndef HAVE_MEMCPY
#   define memcpy(a,b,n) bcopy((b),(a),(n)) // "a" and "b" are reversed.
#endif


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


//
// File I/O
//

#define FILE_HANDLE         int

// FileOpen() access permissions.
#define FACCESS_READ        O_RDONLY
#define FACCESS_WRITE       O_WRONLY
#define FACCESS_READWRITE   O_RDWR

// FileOpen() existence actions.
#define FEXIST_ALWAYS_NEW   O_CREAT|O_TRUNC // TODO: fix
#define FEXIST_NEW          O_CREAT|O_EXCL
#define FEXIST_PRESENT      0
#define FEXIST_REGARDLESS   O_CREAT
#define FEXIST_TRUNCATE     O_TRUNC

// FileOpen() attributes and options.
#define FOPTION_HIDDEN      0
#define FOPTION_RANDOM      0
#define FOPTION_SEQUENTIAL  0

// Value returned by FileOpen() to indicate an error.
#define INVALID_HANDLE_VALUE -1

// FileSeek() methods.
#define FILE_BEGIN          SEEK_SET
#define FILE_CURRENT        SEEK_CUR
#define FILE_END            SEEK_END


//
// Map Windows Base functions to their POSIX equivalents.
//

#ifdef DEBUG
#   define DebugBreak()     (abort())
#else
#   define DebugBreak()     ((void)0)
#endif

#define ExitProcess(code)   (exit(code))

#endif // _PLATUNIX_H_
