/*
 * nxHelper - Tcl extension for nxTools.
 * Copyright (c) 2004-2008 neoxed
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

/*
 * System includes
 */
#define STRSAFE_LIB
#define STRSAFE_NO_CB_FUNCTIONS
#define _WIN32_WINNT 0x0400
#define WIN32_LEAN_AND_MEAN
#include <assert.h>
#include <tchar.h>
#include <windows.h>
#include <shlwapi.h>
#include <strsafe.h>

/*
 * Library includes
 */
#include <tcl.h>
#include <zlib.h>

#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT

/*
 * Local includes
 */
#include "nxMacros.h"
#include "nxMP3Info.h"
#include "nxUtil.h"

/*
 * ObjCmd globals
 */
Tcl_ObjCmdProc Base64ObjCmd;
Tcl_ObjCmdProc KeyObjCmd;
Tcl_ObjCmdProc Mp3ObjCmd;
Tcl_ObjCmdProc SleepObjCmd;
Tcl_ObjCmdProc TimeObjCmd;
Tcl_ObjCmdProc TouchObjCmd;
Tcl_ObjCmdProc VolumeObjCmd;
Tcl_ObjCmdProc ZlibObjCmd;

/*
 * Key globals
 */
Tcl_Mutex keyMutex;
Tcl_HashTable *keyTable;
void KeyClearTable(void);

/*
 * Volume globals
 */
typedef BOOL (WINAPI *Fn_GetDiskFreeSpaceEx)(
    LPCTSTR lpDirectoryName,
    PULARGE_INTEGER lpFreeBytesAvailableToCaller,
    PULARGE_INTEGER lpTotalNumberOfBytes,
    PULARGE_INTEGER lpTotalNumberOfFreeBytes
);

Fn_GetDiskFreeSpaceEx getDiskFreeSpaceExPtr;
OSVERSIONINFO osVersion;

#endif /* _NXHELPER_H_ */
