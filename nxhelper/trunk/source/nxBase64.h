/*
 * nxHelper - Tcl extension for nxTools.
 * Copyright (c) 2004-2006 neoxed
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

Tcl_ObjCmdProc Base64ObjCmd;

#endif /* _NXBASE64_H_ */
