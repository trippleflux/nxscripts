/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2008 neoxed

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

// Disabled warnings
#pragma warning(disable : 4127) // conditional expression is constan
#pragma warning(disable : 4200) // nonstandard extension used : zero-sized array in struct/union
#pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union

// System headers
#define _WIN32_WINNT 0x0403
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <mswsock.h>

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
#include <Timer.h>
#include <GroupFile.h>
#include <Group.h>
#include <UserFile.h>
#include <User.h>

#include <LockObject.h>
#include <VirtualPath.h>
#include <IoOverlapped.h>
#include <IoFile.h>
#include <IoService.h>
#include <IoSocket.h>
#include <IoString.h>
#include <Job.h>
#include <Threads.h>
#include <Event.h>

// MySQL headers
#include <mysql.h>


//
// Macro: ASSERT
//
// Run-time expression assertions, evaluated only in debug builds.
//
#if defined(DEBUG) && !defined(NDEBUG)
#   include "crtdbg.h"
#   define ASSERT       _ASSERTE
#else
#   define ASSERT(expr) ((VOID)0)
#endif

//
// Macro: ELEMENT_COUNT
//
// Determines the number of elements in the specified array.
//
#define ELEMENT_COUNT(array) (sizeof(array) / sizeof(array[0]))

//
// Macro: IS_EOL
//
// Determines if the character is an end-of-line marker.
//
#define IS_EOL(ch)          ((ch) == '\n' || (ch) == '\r')

//
// Macro: IS_SPACE
//
// Determines if the character is whitespace.
//
#define IS_SPACE(ch)        ((ch) == ' ' || (ch) == '\f' || (ch) == '\t' || (ch) == '\v')

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

//
// Macro: INLINE
//
// Inlines the function.
//
#define INLINE __forceinline


// Project headers
#include <proctable.h>
#include <logging.h>
#include <alloc.h>

#endif // BASE_H_INCLUDED
