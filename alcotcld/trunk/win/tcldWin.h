/*++

AlcoTcld - Alcoholicz Tcl daemon.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Windows Platform

Author:
    neoxed (neoxed@gmail.com) Jul 17, 2005

Abstract:
    Windows specific includes and defintions.

--*/

#ifndef _TCLDWIN_H_
#define _TCLDWIN_H_

#ifndef _WINDOWS
#   define _WINDOWS
#endif

// System includes.
#include <windows.h>
#include <shlwapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

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
#   endif
#endif // inline

#include "tcldWinUtil.h"

#endif // _TCLDWIN_H_
