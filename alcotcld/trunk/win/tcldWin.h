/*
 * AlcoTcld - Alcoholicz Tcl daemon.
 * Copyright (c) 2005 Alcoholicz Scripting Team
 *
 * File Name:
 *   tcldWin.h
 *
 * Author:
 *   neoxed (neoxed@gmail.com) July 17, 2005
 *
 * Abstract:
 *   Windows specific includes and defintions.
 */

#ifndef _TCLDWIN_H_
#define _TCLDWIN_H_

#if defined(DEBUG) && !defined(_DEBUG)
#   define _DEBUG
#endif

#ifndef _WINDOWS
#   define _WINDOWS
#endif

/* System includes. */
#include <windows.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <memory.h>
#include <signal.h>
#include <string.h>
#include <wchar.h>

#define STRSAFE_LIB
#define STRSAFE_NO_CB_FUNCTIONS
#include <strsafe.h>

#ifndef inline
#   if (_MSC_VER >= 1200)
#       define inline __forceinline
#   elif defined(_MSC_VER)
#       define inline __inline
#   else
#       error Only MSVC is supported.
#   endif
#endif /* inline */

#endif /* _TCLDWIN_H_ */
