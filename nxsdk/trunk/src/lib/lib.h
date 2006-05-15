/*

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    Library Header

Author:
    neoxed (neoxed@gmail.com) May 13, 2006

Abstract:
    Library common header file.

*/

#ifndef _LIB_H_
#define _LIB_H_

#define STRSAFE_NO_CB_FUNCTIONS
#define WIN32_LEAN_AND_MEAN

// System headers
#include <windows.h>
#include <psapi.h>
#include <strsafe.h>

// Standard headers
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

// SDK headers
#include <nxsdk.h>


#undef ARRAYSIZE
#undef MAX
#undef MIN

// ARRAYSIZE - Returns the number of entries in an array.
#define ARRAYSIZE(a)    (sizeof(a) / sizeof(a[0]))

// MAX - Returns the maximum of two numeric values.
#define MAX(a, b)       (((a) > (b)) ? (a) : (b))

// MIN - Returns the minimum of two numeric values.
#define MIN(a, b)       (((a) < (b)) ? (a) : (b))

// DebugPrint - Display debugging information to stdout.
#if defined(DEBUG) && !defined(NDEBUG)
#   define DebugPrint printf
#else
#   define DebugPrint
#endif

#endif // _LIB_H_
