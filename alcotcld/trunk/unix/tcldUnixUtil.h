/*++

AlcoTcld - Alcoholicz Tcl daemon.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Unix Utilities

Author:
    neoxed (neoxed@gmail.com) Mar 14, 2006

Abstract:
    Unix utility function declarations.

--*/

#ifndef _TCLDUNIXUTIL_H_
#define _TCLDUNIXUTIL_H_

//
// Determine if the given file exists.
//
#define FileExists(path) \
    (access(path, F_OK) == 0)

#endif // _TCLDUNIXUTIL_H_
