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

// System headers
#include <windows.h>
#include <psapi.h>
#include <strsafe.h>

// Standard headers
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <time.h>

// SDK headers
#include <nxsdk.h>


// ARRAYSIZE - Returns the number of entries in an array.
#undef ARRAYSIZE
#define ARRAYSIZE(a)    (sizeof(a) / sizeof(a[0]))

// DebugPrint - Display debugging information to stdout.
#if defined(DEBUG) && !defined(NDEBUG)
#   define DebugPrint printf
#else
#   define DebugPrint
#endif

#endif // _LIB_H_
