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

/* Unicode macros. */
#if defined(_UNICODE) && !defined(UNICODE)
#define UNICODE
#elif defined(UNICODE) && !defined(_UNICODE)
#define _UNICODE
#endif

/* Windows includes. */
#define _WIN32_WINNT 0x0400
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>
#pragma comment (lib,"shlwapi.lib")

/* Common includes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <tchar.h>
#include <time.h>

#define STRSAFE_LIB
#define STRSAFE_NO_CB_FUNCTIONS
#include <strsafe.h>

/* Library includes. */
#ifndef TCL_THREADS
#   error "TCL_THREADS not defined."
#endif
#include <tcl.h>
#include <zlib.h>
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT

/* nxHelper includes. */
#include <nxMacros.h>
#include <nxMP3Info.h>

#include <nxBase64.h>
#include <nxMP3.h>
#include <nxTime.h>
#include <nxTouch.h>
#include <nxUtil.h>
#include <nxVar.h>
#include <nxVolume.h>
#include <nxZlib.h>

typedef BOOL (WINAPI *Fn_GetDiskFreeSpaceEx)(
    LPCTSTR lpDirectoryName,
    PULARGE_INTEGER lpFreeBytesAvailableToCaller,
    PULARGE_INTEGER lpTotalNumberOfBytes,
    PULARGE_INTEGER lpTotalNumberOfFreeBytes
);

/* "::nx::volume" globals */
Fn_GetDiskFreeSpaceEx getDiskFreeSpaceExPtr;
OSVERSIONINFO osVersion;

/* "::nx::var" globals */
Tcl_Mutex varMutex;
Tcl_HashTable *varTable;

#endif /* _NXHELPER_H_ */
