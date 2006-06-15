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

#undef UNICODE
#undef _UNICODE

// System headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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

// Project headers
#include "proctable.h"
#include "queue.h"

#include "backends.h"
#include "condvars.h"
#include "dbconn.h"
#include "pool.h"
#include "utils.h"

// Calling convention used by ioFTPD for module functions.
#define MODULE_CALL __cdecl


#undef ARRAYSIZE
#undef ASSERT
#undef STRINGIFY
#undef _STRINGIFY

// ASSERT - Expression assertion.
#if defined(DEBUG) && !defined(NDEBUG)
#   include "crtdbg.h"
#   define ASSERT _ASSERTE
#else
#   define ASSERT
#endif

// ARRAYSIZE - Returns the number of entries in an array.
#define ARRAYSIZE(a) (sizeof(a) / sizeof(a[0]))

// DebugHead  - Print debug header.
// DebugPrint - Print debug message.
// DebugFoot  - Print debug footer.
#if defined(DEBUG) && !defined(NDEBUG)
#   ifdef DEBUG_FILE
#       define DebugHead  FileHeader
#       define DebugPrint FileMessage
#       define DebugFoot  FileFooter
#   else
#       define DebugHead  DebuggerHeader
#       define DebugPrint DebuggerMessage
#       define DebugFoot  DebuggerFooter
#   endif
#else
#   define DebugHead
#   define DebugPrint
#   define DebugFoot
#endif

// STRINGIFY - Wraps an argument in quotes.
#define STRINGIFY(a) _STRINGIFY(a)
#define _STRINGIFY(a) #a


// Calling convention used by ioFTPD for module functions.
#define MODULE_CALL __cdecl

#endif // _MYDB_H_
