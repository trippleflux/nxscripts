/*++

AlcoTcld - Alcoholicz Tcl daemon.
Copyright (c) 2005 Alcoholicz Scripting Team

Module Name:
    tcld.h

Author:
    neoxed (neoxed@gmail.com) July 17, 2005

Abstract:
    Common include file.

--*/

#ifndef _TCLD_H_
#define _TCLD_H_

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#include <tcl.h>

#if defined(_WIN32) || defined(_WIN64) || defined(_WINDOWS)
#   include "../win/tcldWin.h"
#else
#   include "../unix/tcldUnix.h"
#endif

#ifndef TRUE
#   define TRUE  1
#endif
#ifndef FALSE
#   define FALSE 0
#endif

// ARRAYSIZE - Returns the number of entries in an array.
#undef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a) / sizeof(a[0]))

// DEBUGLOG - Write a message to the debug log.
#ifdef DEBUG
void DebugLog(const char *format, ...);
#   define DEBUGLOG DebugLog
#else
#   define DEBUGLOG
#endif

Tcl_Interp *
TclInit(
    int argc,
    char **argv,
    int service,
    Tcl_ExitProc *exitProc
    );

#endif // _TCLD_H_
