/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Common Header

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Common header, used by all source files.

*/

#ifndef _MYDB_H_
#define _MYDB_H_

#if (_MSC_VER < 1400)
#   error You must be using VC2005, or newer, to compile this application.
#endif

#undef UNICODE
#undef _UNICODE

// System headers
#define _WIN32_WINNT 0x0403
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>

#define STRSAFE_LIB
#define STRSAFE_NO_CB_FUNCTIONS
#include <strsafe.h>

// Standard headers
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

// ioFTPD headers
#include <ServerLimits.h>
#include <Buffer.h>
#include <Log.h>
#include <Timer.h>
#include <UserFile.h>
#include <GroupFile.h>
#include <User.h>
#include <Group.h>

// MySQL headers
#include <mysql.h>

// Project headers
#include "proctable.h"
#include "queue.h"
#include "db.h"
#include "debug.h"
#include "backends.h"
#include "condvar.h"
#include "pool.h"


//
// Macro: ASSERT
//
// Run-time expression assertions, evaluated only in debug builds.
//
#if defined(DEBUG) && !defined(NDEBUG)
#   include "crtdbg.h"
#   define ASSERT   _ASSERTE
#else
#   define ASSERT
#endif

//
// Macros:
//
// TRACE_HEAD - Log debug header.
// TRACE_FOOT - Log debug footer.
// TRACE      - Log debug message.
//
#if defined(DEBUG) && !defined(NDEBUG)
#   ifdef DEBUG_FILE
#       define TRACE_HEAD()         TraceFileHeader()
#       define TRACE_FOOT()         TraceFileFooter()
#       define TRACE(format, ...)   TraceFileFormat(__FUNCTION__, format, __VA_ARGS__)
#   else
#       define TRACE_HEAD()         TraceDebugHeader()
#       define TRACE_FOOT()         TraceDebugFooter()
#       define TRACE(format, ...)   TraceDebugFormat(__FUNCTION__, format, __VA_ARGS__)
#   endif
#else
#   define TRACE_HEAD()             ((void)0)
#   define TRACE_FOOT()             ((void)0)
#   define TRACE(format, ...)       ((void)0)
#endif

//
// Macro: ELEMENT_COUNT
//
// Determines the number of elements in the specified array.
//
#define ELEMENT_COUNT(array) (sizeof(array) / sizeof(array[0]))

//
// Macro: inline
//
// Inline the function during compilation.
//
#define inline __forceinline

//
// Macro: MAX
//
// Returns the maximum of two values.
//
#define MAX(a, b)           (((a) > (b)) ? (a) : (b))

//
// Macro: MIN
//
// Returns the minimum of two values.
//
#define MIN(a, b)           (((a) < (b)) ? (a) : (b))

//
// Macro: STRINGIFY
//
// Quotes a value as a string in the C preprocessor.
//
#define STRINGIFY(s)        STRINGIFY_HELPER(s)
#define STRINGIFY_HELPER(s)  #s

#endif // _MYDB_H_
