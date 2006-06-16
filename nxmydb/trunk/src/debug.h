/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Utilities

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Miscellaneous debugging utilities.

*/

#ifndef _DEBUG_H_
#define _DEBUG_H_

#undef ASSERT
#undef DebugHead
#undef DebugPrint
#undef DebugFoot

// DEBUG and NDEBUG are mutually exclusive
#ifdef NDEBUG
#   undef DEBUG
#endif

// ASSERT - Expression assertions.
#ifdef DEBUG
#   include "crtdbg.h"
#   define ASSERT _ASSERTE
#else
#   define ASSERT
#endif

// DebugHead  - Log debug header.
// DebugPrint - Log debug message.
// DebugFoot  - Log debug footer.
#ifdef DEBUG
#   ifdef DEBUG_FILE
#       define DebugHead()                      LogFileHeader()
#       define DebugPrint(funct, format, ...)   LogFileFormat(funct, format, __VA_ARGS__)
#       define DebugFoot()                      LogFileFooter()
#   else
#       define DebugHead()                      LogDebuggerHeader()
#       define DebugPrint(funct, format, ...)   LogDebuggerFormat(funct, format, __VA_ARGS__)
#       define DebugFoot()                      LogDebuggerFooter()
#   endif
#else
#   define DebugHead()                          ((void)0)
#   define DebugPrint(funct, format, ...)       ((void)0)
#   define DebugFoot()                          ((void)0)
#endif


//
// Debugger output
//

#ifdef DEBUG
void
LogDebuggerHeader(
    void
    );

void
LogDebuggerFormat(
    const char *funct,
    const char *format,
    ...
    );

void
LogDebuggerFooter(
    void
    );
#endif // DEBUG


//
// File output
//

#ifdef DEBUG
void
LogFileHeader(
    void
    );

void
LogFileFormat(
    const char *funct,
    const char *format,
    ...
    );

void
LogFileFooter(
    void
    );
#endif // DEBUG

#endif // _DEBUG_H_
