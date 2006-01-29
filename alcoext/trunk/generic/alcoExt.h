/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    alcoExt.h

Author:
    neoxed (neoxed@gmail.com) Apr 16, 2005

Abstract:
    Common header file that includes all necessary headers.

--*/

#ifndef _ALCOEXT_H_
#define _ALCOEXT_H_

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#include <tcl.h>

// These definitions are only present in headers for Tcl 8.5, or newer.
#ifndef TCL_UNLOAD_DETACH_FROM_INTERPRETER
#define TCL_UNLOAD_DETACH_FROM_INTERPRETER   (1<<0)
#define TCL_UNLOAD_DETACH_FROM_PROCESS       (1<<1)

typedef int (Tcl_PackageUnloadProc) _ANSI_ARGS_((Tcl_Interp *interp, int flags));
#endif // TCL_UNLOAD_DETACH_FROM_INTERPRETER

#if defined(_WIN32) || defined(_WIN64) || defined(_WINDOWS)
#   include "../win/alcoWin.h"
#else
#   include "../unix/alcoUnix.h"
#endif

// Extension state structure.
typedef struct {
    Tcl_HashTable *cryptTable;  // Table of hash and PRNG handles.
#ifndef _WINDOWS
    Tcl_HashTable *glftpdTable; // Table of glftpd handles.
#endif // !_WINDOWS
} ExtState;

typedef struct StateList {
    Tcl_Interp *interp;
    ExtState *state;
    struct StateList *next;
    struct StateList *prev;
} StateList;

#include <tomcrypt.h>

#include "alcoCompress.h"
#include "alcoCrypt.h"
#include "alcoEncoding.h"
#include "alcoUtil.h"
#include "alcoVolume.h"

EXTERN Tcl_PackageInitProc   Alcoext_Init;
EXTERN Tcl_PackageInitProc   Alcoext_SafeInit;
EXTERN Tcl_PackageUnloadProc Alcoext_Unload;
EXTERN Tcl_PackageUnloadProc Alcoext_SafeUnload;

#endif // _ALCOEXT_H_
