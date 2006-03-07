/*
 * nxHelper - Tcl extension for nxTools.
 * Copyright (c) 2005 neoxed
 *
 * File Name:
 *   nxVar.h
 *
 * Author:
 *   neoxed (neoxed@gmail.com) Mar 7, 2006
 *
 * Abstract:
 *   Variable command definitions.
 */

#ifndef _NXVAR_H_
#define _NXVAR_H_

void
VarFree(
    Tcl_HashTable *tablePtr
    );

Tcl_ObjCmdProc VarObjCmd;

#endif /* _NXVAR_H_ */
