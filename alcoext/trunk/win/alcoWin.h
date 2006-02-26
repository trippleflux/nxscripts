/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Windows Platform

Author:
    neoxed (neoxed@gmail.com) Apr 16, 2005

Abstract:
    Windows specific headers and macros.

--*/

#ifndef _ALCOWIN_H_
#define _ALCOWIN_H_

#ifndef _WINDOWS
#   define _WINDOWS
#endif

// Windows linkage.
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT

// System includes.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define STRSAFE_LIB
#define STRSAFE_NO_CB_FUNCTIONS
#include <strsafe.h>

#if (_MSC_VER == 1400)
//
// The compiler in VS2005 (VC8.0) appears to generate defective code for a few
// functions in the "alcoWinIoFtpd.c" source file; VS2003 (VC7.1) is recommended.
//
// When compiled with VS2005, the GetOnlineFields() function gets caught in an
// infinite loop due to strange return values from ShmQuery(). However, when
// compiled with VS2003 the functions work flawlessly. It may be a bug on my
// part, but until I know for certain, VS2003 is recommended.
//
#error "Due to bugs in the VS2005 compiler it is not supported, use VS2003 instead."
#endif

#ifndef inline
#   if (_MSC_VER >= 1200)
#       define inline __forceinline
#   elif defined(_MSC_VER)
#       define inline __inline
#   else
#       define inline
#   endif // _MSC_VER
#endif // inline

#include "alcoWinIoFtpd.h"
#include "alcoWinUtil.h"

// Global variables.
WinProcs winProcs;
OSVERSIONINFOA osVersion;

#endif // _ALCOWIN_H_
