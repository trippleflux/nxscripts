/*
 * nxHelper - Tcl extension for nxTools.
 * Copyright (c) 2004-2006 neoxed
 *
 * File Name:
 *   nxUtil.h
 *
 * Author:
 *   neoxed (neoxed@gmail.com) May 22, 2005
 *
 * Abstract:
 *   Miscellanenous utilities.
 */

#ifndef _NXUTIL_H_
#define _NXUTIL_H_

char *
TclSetWinError(
    Tcl_Interp *interp,
    DWORD errorCode
    );

int
TclSwitchCompare(
    Tcl_Obj *objPtr,
    const char *switchName
    );

int
TclGetPathFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,
    Tcl_DString *buffer
    );


BOOL
GetTimeZoneBias(
    long *bias
    );

unsigned long
FileTimeToPosixEpoch(
    const FILETIME *fileTime
    );

void
PosixEpochToFileTime(
    unsigned long epochTime,
    FILETIME *fileTime
    );

#endif /* _NXUTIL_H_ */
