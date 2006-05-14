/*

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    Common Include

Author:
    neoxed (neoxed@gmail.com) May 13, 2006

Abstract:
    Common include file.

*/

#ifndef _LIB_H_
#define _LIB_H_

// System includes
#include <windows.h>
#include <psapi.h>
#include <strsafe.h>

// Standard includes
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <time.h>

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
