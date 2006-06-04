/*

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    Utilities

Author:
    neoxed (neoxed@gmail.com) Jun 4, 2006

Abstract:
    Miscellaneous utilities.

*/

#ifndef _UTIL_H_
#define _UTIL_H_

void
OutputDebugger(
    const char *format,
    ...
    );

void
OutputFile(
    const char *format,
    ...
    );

#ifdef DEBUG
// OutputDebugger or OutputFile
#   define DebugPrint OutputDebugger
#else
#   define DebugPrint
#endif

#endif // _UTIL_H_
