/*
 * nxHelper - Tcl extension for nxTools.
 * Copyright (c) 2004-2006 neoxed
 *
 * File Name:
 *   nxTime.h
 *
 * Author:
 *   neoxed (neoxed@gmail.com) May 22, 2005
 *
 * Abstract:
 *   Time command definitions.
 */

#ifndef _NXTIME_H_
#define _NXTIME_H_

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

Tcl_ObjCmdProc TimeObjCmd;

#endif /* _NXTIME_H_ */
