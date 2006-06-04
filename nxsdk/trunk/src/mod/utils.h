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

#ifdef DEBUG
void
OutputDebugger(
    const char *format,
    ...
    );
#endif // DEBUG

#ifdef DEBUG
void
OutputFile(
    const char *format,
    ...
    );
#endif // DEBUG

#endif // _UTIL_H_
