/*
 * nxHelper - Tcl extension for nxTools.
 * Copyright (c) 2005 neoxed
 *
 * File Name:
 *   nxBase64.h
 *
 * Author:
 *   neoxed (neoxed@gmail.com) May 22, 2005
 *
 * Abstract:
 *   Base64 command definitions.
 */

#ifndef _NXBASE64_H_
#define _NXBASE64_H_

#define BASE64_SUCCESS  0
#define BASE64_INVALID  1
#define BASE64_OVERFLOW 2

int Base64ObjCmd(ClientData dummy, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

#endif /* _NXBASE64_H_ */
