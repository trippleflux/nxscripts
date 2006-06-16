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
#       define DebugHead  LogFileHeader
#       define DebugPrint LogFileFormat
#       define DebugFoot  LogFileFooter
#   else
#       define DebugHead  LogDebuggerHeader
#       define DebugPrint LogDebuggerFormat
#       define DebugFoot  LogDebuggerFooter
#   endif
#else
#   define DebugHead()
#   define DebugPrint
#   define DebugFoot()
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
