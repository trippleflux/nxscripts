/*
 * Infernus Library - Tcl extension for the Infernus sitebot.
 * Copyright (c) 2005 Infernus Development Team
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

#ifndef __NXHELPER_H__
#define __NXHELPER_H__

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
#include <tchar.h>
#include <time.h>

/* Library includes. */
#include <tcl.h>
#include <zlib.h>

#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT

/* Safe string functions. */
#define STRSAFE_LIB
#define STRSAFE_NO_CB_FUNCTIONS
#include <strsafe.h>

/* nxHelper includes. */
#include <nxMacros.h>
//#include <nxMP3Info.h>

/* Function includes. */
//#include <nxBase64.h>
//#include <nxMP3.h>
#include <nxTime.h>
#include <nxTouch.h>
//#include <nxVolume.h>
#include <nxZlib.h>

#endif /* __NXHELPER_H__ */
