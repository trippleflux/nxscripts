/*++

AlcoTcld - Alcoholicz Tcl daemon.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Common

Author:
    neoxed (neoxed@gmail.com) Jul 17, 2005

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


void
LogError(
    const char *format,
    ...
    );

void
LogErrorObj(
    const char *message,
    Tcl_Obj *objPtr
    );

Tcl_Interp *
TclInit(
    int argc,
    char **argv,
    int service,
    Tcl_ExitProc *exitProc
    );


// True when the process is in the background.
int inBackground;

#endif // _TCLD_H_
