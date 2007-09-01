/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    Common Header

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Common header, used by all source files.

*/

#ifndef BASE_H_INCLUDED
#define BASE_H_INCLUDED

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
#include <IoError.h>
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


//
// Macro: ASSERT
//
// Run-time expression assertions, evaluated only in debug builds.
//
#if defined(DEBUG) && !defined(NDEBUG)
#   include "crtdbg.h"
#   define ASSERT   _ASSERTE
#else
#   define ASSERT   ((VOID)0)
#endif

//
// Macro: ELEMENT_COUNT
//
// Determines the number of elements in the specified array.
//
#define ELEMENT_COUNT(array) (sizeof(array) / sizeof(array[0]))

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

//
// TRACE_HEAD - Log debug header.
// TRACE_FOOT - Log debug footer.
// TRACE      - Log debug message.
//
#if defined(DEBUG) && !defined(NDEBUG)
#   define TRACE_HEAD()         TraceHeader()
#   define TRACE_FOOT()         TraceFooter()
#   define TRACE(format, ...)   TraceFormat(__FUNCTION__, format, __VA_ARGS__)
#else
#   define TRACE_HEAD()         ((VOID)0)
#   define TRACE_FOOT()         ((VOID)0)
#   define TRACE(format, ...)   ((VOID)0)
#endif


//
// Macro: CCALL
//
// C calling convention.
//
#define CCALL   __cdecl

//
// Macro: FCALL
//
// Fast-call calling convention.
//
#define FCALL   __fastcall

//
// Macro: SCALL
//
// Standard-call calling convention.
//
#define SCALL   __stdcall


// Project headers
#include <debug.h>
#include <proctable.h>

#endif // BASE_H_INCLUDED
