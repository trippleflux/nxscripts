/*
 * nxHelper - Tcl extension for nxTools.
 * Copyright (c) 2004-2006 neoxed
 *
 * File Name:
 *   nxHelper.h
 *
 * Author:
 *   neoxed (neoxed@gmail.com) May 22, 2005
 *
 * Abstract:
 *   Common include file.
 */

#ifndef _NXHELPER_H_
#define _NXHELPER_H_

#define UNICODE
#define _UNICODE

/* Windows includes. */
#define _WIN32_WINNT 0x0400
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>

/* Common includes. */
#include <assert.h>
#include <string.h>
#include <tchar.h>

#define STRSAFE_LIB
#define STRSAFE_NO_CB_FUNCTIONS
#include <strsafe.h>

/* Library includes. */
#include <tcl.h>
#include <zlib.h>

#ifndef TCL_THREADS
#   error "TCL_THREADS not defined."
#endif
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT

/* nxHelper includes. */
#include "nxMacros.h"
#include "nxMP3Info.h"
#include "nxUtil.h"

/* Object commands */
Tcl_ObjCmdProc Base64ObjCmd;
Tcl_ObjCmdProc KeyObjCmd;
Tcl_ObjCmdProc Mp3ObjCmd;
Tcl_ObjCmdProc SleepObjCmd;
Tcl_ObjCmdProc TimeObjCmd;
Tcl_ObjCmdProc TouchObjCmd;
Tcl_ObjCmdProc VolumeObjCmd;
Tcl_ObjCmdProc ZlibObjCmd;

/* "::nx::key" globals */
Tcl_Mutex keyMutex;
Tcl_HashTable *keyTable;
void KeyClearTable(void);

/* "::nx::volume" globals */
typedef BOOL (WINAPI *Fn_GetDiskFreeSpaceEx)(
    LPCTSTR lpDirectoryName,
    PULARGE_INTEGER lpFreeBytesAvailableToCaller,
    PULARGE_INTEGER lpTotalNumberOfBytes,
    PULARGE_INTEGER lpTotalNumberOfFreeBytes
);

Fn_GetDiskFreeSpaceEx getDiskFreeSpaceExPtr;
OSVERSIONINFO osVersion;

#endif /* _NXHELPER_H_ */
