#ifndef __NXHELPER_H__
#define __NXHELPER_H__

// Windows includes
#define _WIN32_WINNT 0x0400
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ShlWapi.h>
#pragma comment (lib,"shlwapi.lib")

// Common includes
#include <StdIo.h>
#include <StdLib.h>
#include <String.h>
#include <TChar.h>
#include <Time.h>

// Safe string functions
#define STRSAFE_LIB
#define STRSAFE_NO_CB_FUNCTIONS
#include <StrSafe.h>

// Library includes
#define USE_TCL_STUBS
#include <Tcl.h>
#include <Zlib.h>

// Function includes
#include <Constants.h>

#include <Base64.h>
#include <Compress.h>
#include <Free.h>
#include <MP3.h>
#include <Times.h>
#include <Touch.h>

#endif // __NXHELPER_H__
