/*++

AlcoTcld - Alcoholicz Tcl daemon.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Windows Utilities

Author:
    neoxed (neoxed@gmail.com) Mar 13, 2006

Abstract:
    Windows utility function declarations.

--*/

#ifndef _TCLDWINUTIL_H_
#define _TCLDWINUTIL_H_

#ifdef DEBUG
void
DebugPrint(
    const char *format,
    ...
    );
#else
#   define DebugPrint
#endif

//
// Determine if the given file exists.
//
#define FileExists(path) \
    (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES)

#endif // _TCLDWINUTIL_H_
