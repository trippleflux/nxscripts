/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005 Alcoholicz Scripting Team

Module Name:
    alcoWin.h

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define STRSAFE_LIB
#define STRSAFE_NO_CB_FUNCTIONS
#include <strsafe.h>

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
