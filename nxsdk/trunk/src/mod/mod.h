/*

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    Common Header

Author:
    neoxed (neoxed@gmail.com) Jun 4, 2006

Abstract:
    Common header, used by all source files.

*/

#ifndef _MOD_H_
#define _MOD_H_

#undef UNICODE
#undef _UNICODE

// System headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STRSAFE_NO_CB_FUNCTIONS
#include <strsafe.h>

// Standard headers
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

// ioFTPD headers
#include <ServerLimits.h>
#include <Buffer.h>
#include <Log.h>
#include <Timer.h>
#include <UserFile.h>
#include <GroupFile.h>
#include <User.h>
#include <Group.h>

// Project headers
#include "proctable.h"
#include "utils.h"


// Calling convention used by ioFTPD for module functions.
#define MODULE_CALL __cdecl

//
// Debug message printing.
//
// OutputDebugger - Writes message to debugger.
// OutputFile     - Writes message to log file.
//
#if defined(DEBUG) && !defined(NDEBUG)
#   define DebugPrint OutputDebugger
#else
#   define DebugPrint
#endif

#endif // _MOD_H_
