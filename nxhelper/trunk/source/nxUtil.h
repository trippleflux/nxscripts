/*
 * nxHelper - Tcl extension for nxTools.
 * Copyright (c) 2005 neoxed
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

#ifndef __NXUTIL_H__
#define __NXUTIL_H__

char *TclSetWinError(Tcl_Interp *interp, unsigned long errorCode);
int PartialSwitchCompare(Tcl_Obj *objPtr, const char *switchName);
int SleepObjCmd(ClientData dummy, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

#endif /* __NXUTIL_H__ */
