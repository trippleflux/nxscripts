/*
 * AlcoExt - Alcoholicz Tcl extension.
 * Copyright (c) 2005 Alcoholicz Scripting Team
 *
 * File Name:
 *   alcoWin.h
 *
 * Author:
 *   neoxed (neoxed@gmail.com) April 16, 2005
 *
 * Abstract:
 *   Windows specific includes, macros, and function declarations.
 */

#ifndef __ALCOWIN_H__
#define __ALCOWIN_H__

/* Windows linkage. */
#ifdef TCL_STORAGE_CLASS
#undef TCL_STORAGE_CLASS
#endif /* TCL_STORAGE_CLASS */
#define TCL_STORAGE_CLASS DLLEXPORT

/* System includes. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <time.h>

#define STRSAFE_LIB
#define STRSAFE_NO_CB_FUNCTIONS
#include <strsafe.h>

#ifndef inline
#if (_MSC_VER >= 1200)
#define inline __forceinline
#elif defined(_MSC_VER)
#define inline __inline
#else /* _MSC_VER */
#error Only MSVC is supported.
#endif /* _MSC_VER */
#endif /* inline */

#include "alcoWinIoFtpd.h"
#include "alcoWinProcs.h"
#include "alcoWinUtil.h"

/* Global variables. */
WinProcs winProcs;
OSVERSIONINFOA osVersion;

#endif /* __ALCOWIN_H__ */
