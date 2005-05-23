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

#ifndef __NXBASE64_H__
#define __NXBASE64_H__

#define BASE64_SUCCESS  0
#define BASE64_INVALID  1
#define BASE64_OVERFLOW 2

int Base64ObjCmd(ClientData dummy, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

#endif /* __NXBASE64_H__ */
