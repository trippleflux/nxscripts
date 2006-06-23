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

#ifndef _PLATWIN_H_
#define _PLATWIN_H_

#define LITTLE_ENDIAN 1
#define WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

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
// Performance Counter
//

typedef struct {
    UINT64 frequency;
    UINT64 start;
    UINT64 stop;
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
	    counter->start = (UINT64)GetTickCount();
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
	    counter->stop = (UINT64)GetTickCount();
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

#endif // _PLATWIN_H_
