#ifndef __NXHELPER_H__
#define __NXHELPER_H__

#define UNICODE

// Unicode macros
#if defined(_UNICODE) && !defined(UNICODE)
#define UNICODE
#elif defined(UNICODE) && !defined(_UNICODE)
#define _UNICODE
#endif

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
//#define STRSAFE_LIB
#define STRSAFE_NO_CB_FUNCTIONS
#include <StrSafe.h>

// Library includes
#include <MP3Info.h>
#define USE_TCL_STUBS
#include <Tcl.h>
#include <Zlib.h>

// nxHelper includes
#include <Constants.h>
#include <Macros.h>

// Function includes
#include <Base64.h>
#include <Compress.h>
#include <MP3.h>
#include <Times.h>
#include <Touch.h>
#include <Volume.h>

#endif // __NXHELPER_H__
