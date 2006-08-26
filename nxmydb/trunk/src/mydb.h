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
// Macro: Assert
//
// Run-time expression assertions, evaluated only in debug builds.
//
#if defined(DEBUG) && !defined(NDEBUG)
#   include "crtdbg.h"
#   define Assert _ASSERTE
#else
#   define Assert
#endif

//
// Macros:
//
// DebugHead  - Log debug header.
// DebugPrint - Log debug message.
// DebugFoot  - Log debug footer.
//
#if defined(DEBUG) && !defined(NDEBUG)
#   ifdef DEBUG_FILE
#       define DebugHead()                      LogFileHeader()
#       define DebugPrint(funct, format, ...)   LogFileFormat(funct, format, __VA_ARGS__)
#       define DebugFoot()                      LogFileFooter()
#   else
#       define DebugHead()                      LogDebuggerHeader()
#       define DebugPrint(funct, format, ...)   LogDebuggerFormat(funct, format, __VA_ARGS__)
#       define DebugFoot()                      LogDebuggerFooter()
#   endif
#else
#   define DebugHead()                          ((void)0)
#   define DebugPrint(funct, format, ...)       ((void)0)
#   define DebugFoot()                          ((void)0)
#endif

//
// Macro: ElementCount
//
// Determines the number of elements in the specified array.
//
#define ElementCount(array) (sizeof(array) / sizeof(array[0]))

//
// Macro: inline
//
// Inline the function during compilation.
//
#define inline __forceinline

//
// Macro: Max
//
// Returns the maximum of two values.
//
#define Max(a, b)           (((a) > (b)) ? (a) : (b))

//
// Macro: Min
//
// Returns the minimum of two values.
//
#define Min(a, b)           (((a) < (b)) ? (a) : (b))

//
// Macro: Stringify
//
// Quotes a value as a string in the C preprocessor.
//
#define Stringify(s)        StringifyHelper(s)
#define StringifyHelper(s)  #s


// Calling convention used by ioFTPD for module functions.
#define MODULE_CALL __cdecl

#endif // _MYDB_H_
