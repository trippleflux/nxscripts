/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Windows Platform

Author:
    neoxed (neoxed@gmail.com) Jul 17, 2005

Abstract:
    Windows specific includes and definitions.

--*/

#ifndef _ALCOPLATWIN_H_
#define _ALCOPLATWIN_H_

#define LITTLE_ENDIAN 1
#define WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <tchar.h>
#include <wchar.h>

#if !defined(__cplusplus)
#   undef inline
#   if (_MSC_VER >= 1200)
#       define inline __forceinline
#   elif defined(_MSC_VER)
#       define inline __inline
#   else
#       define inline
#   endif
#endif // inline


//
// Data types and related definitions.
//

#define PATH_LENGTH         MAX_PATH

typedef unsigned char       bool_t;
typedef unsigned char       byte_t;
typedef TCHAR               tchar_t;

typedef signed __int8       int8_t;
typedef signed __int16      int16_t;
typedef signed __int32      int32_t;
typedef signed __int64      int64_t;
typedef unsigned __int8     uint8_t;
typedef unsigned __int16    uint16_t;
typedef unsigned __int32    uint32_t;
typedef unsigned __int64    uint64_t;


//
// Performance Counter
//

typedef struct {
    uint64_t frequency;
    uint64_t start;
    uint64_t stop;
} PERF_COUNTER;

inline
void
PerfCounterStart(
    PERF_COUNTER *counter
    )
{
    //
    // If the current hardware does not support performance
    // counters, fallback to GetTickCount.
    //
    if (QueryPerformanceFrequency((LARGE_INTEGER *)&counter->frequency)) {
	    QueryPerformanceCounter((LARGE_INTEGER *)&counter->start);
	} else {
	    counter->start = (uint64_t)GetTickCount();
	    counter->frequency = 0;
	}
	counter->stop = 0;
}

inline
void
PerfCounterStop(
    PERF_COUNTER *counter
    )
{
    if (counter->frequency) {
	    QueryPerformanceCounter((LARGE_INTEGER *)&counter->stop);
	} else {
	    counter->stop = (uint64_t)GetTickCount();
	}
}

inline
double
PerfCounterDiff(
    PERF_COUNTER *counter
    )
{
    if (counter->frequency) {
        return ((double)(counter->stop - counter->start)) / ((double)counter->frequency/1000.0);
	} else {
	    return (double)(counter->stop - counter->start);
	}
}


//
// File I/O
//

#define FILE_HANDLE         HANDLE

// FileOpen() access permissions.
#define FACCESS_READ        GENERIC_READ
#define FACCESS_WRITE       GENERIC_WRITE
#define FACCESS_READWRITE   GENERIC_READ|GENERIC_WRITE

// FileOpen() existence actions.
#define FEXIST_ALWAYS_NEW   CREATE_ALWAYS
#define FEXIST_NEW          CREATE_NEW
#define FEXIST_PRESENT      OPEN_EXISTING
#define FEXIST_REGARDLESS   OPEN_ALWAYS
#define FEXIST_TRUNCATE     TRUNCATE_EXISTING

// FileOpen() attributes and options.
#define FOPTION_HIDDEN      FILE_ATTRIBUTE_HIDDEN
#define FOPTION_RANDOM      FILE_FLAG_RANDOM_ACCESS
#define FOPTION_SEQUENTIAL  FILE_FLAG_SEQUENTIAL_SCAN


//
// Map awkwardly named CRT functions (plenty more to add...).
//

#define wfopen          _wfopen

#define snprintf        _snprintf
#define snwprintf       _snwprintf
#define vsnprintf       _vsnprintf
#define vsnwprintf      _vsnwprintf

#define strcasecmp      _stricmp
#define wcscasecmp      _wcsicmp
#define strncasecmp     _strnicmp
#define wcsncasecmp     _wcsnicmp

#endif // _ALCOPLATWIN_H_
