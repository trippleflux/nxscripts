/*
 * AlcoExt - Alcoholicz Tcl extension.
 * Copyright (c) 2005 Alcoholicz Scripting Team
 *
 * File Name:
 *   alcoExt.h
 *
 * Author:
 *   neoxed (neoxed@gmail.com) April 16, 2005
 *
 * Abstract:
 *   Common include file.
 */

#ifndef __ALCOEXT_H__
#define __ALCOEXT_H__

#include <tcl.h>

/* These definitions are only present in headers for Tcl 8.5, or newer. */
#ifndef TCL_UNLOAD_DETACH_FROM_INTERPRETER
#define TCL_UNLOAD_DETACH_FROM_INTERPRETER   (1<<0)
#define TCL_UNLOAD_DETACH_FROM_PROCESS       (1<<1)

typedef int (Tcl_PackageUnloadProc) _ANSI_ARGS_((Tcl_Interp *interp, int flags));
#endif /* TCL_UNLOAD_DETACH_FROM_INTERPRETER */

/* Extension state structure. */
typedef struct {
    unsigned long cryptHandle;
    Tcl_HashTable *cryptTable;

#ifdef __WIN32__
    unsigned long ioHandle;
    Tcl_HashTable *ioTable;
#else /* __WIN32__ */
    unsigned long glHandle;
    Tcl_HashTable *glTable;
#endif /* __WIN32__ */

} ExtState;

typedef struct StateList {
    Tcl_Interp *interp;
    ExtState *state;
    struct StateList *next;
    struct StateList *prev;
} StateList;

#ifdef __WIN32__
#include "../win/alcoWin.h"
#else /* __WIN32__ */
#include "../unix/alcoUnix.h"
#endif /* __WIN32__ */

#include <tomcrypt.h>
#include <zlib.h>

#include "alcoCrypt.h"
#include "alcoEncoding.h"
#include "alcoUtil.h"
#include "alcoVolume.h"
#include "alcoZlib.h"

EXTERN Tcl_PackageInitProc   Alcoext_Init;
EXTERN Tcl_PackageInitProc   Alcoext_SafeInit;
EXTERN Tcl_PackageUnloadProc Alcoext_Unload;
EXTERN Tcl_PackageUnloadProc Alcoext_SafeUnload;

#endif /* __ALCOEXT_H__ */
