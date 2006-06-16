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

#include "dbconn.h"
#include "debug.h"
#include "backends.h"
#include "condvars.h"
#include "pool.h"


#undef ARRAYSIZE
#undef INLINE
#undef STRINGIFY
#undef _STRINGIFY

// ARRAYSIZE - Returns the number of entries in an array.
#define ARRAYSIZE(a) (sizeof(a) / sizeof(a[0]))

// INLINE - Inline the function during compilation.
#if (_MSC_VER >= 1200)
#    define INLINE __forceinline
#elif defined(_MSC_VER)
#    define INLINE __inline
#else
#    define INLINE
#endif

// STRINGIFY - Wraps an argument in quotes.
#define STRINGIFY(a)  _STRINGIFY(a)
#define _STRINGIFY(a) #a


// Calling convention used by ioFTPD for module functions.
#define MODULE_CALL __cdecl

#endif // _MYDB_H_
