/*
 * nxHelper - Tcl extension for nxTools.
 * Copyright (c) 2005 neoxed
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

#ifndef __NXTIME_H__
#define __NXTIME_H__

BOOL GetTimeZoneBias(long *bias);
unsigned long FileTimeToPosixEpoch(const FILETIME *fileTime);
void PosixEpochToFileTime(unsigned long epochTime, FILETIME *fileTime);

int TimeObjCmd(ClientData dummy, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

#endif /* __NXTIME_H__ */
