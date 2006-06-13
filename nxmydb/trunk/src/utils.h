/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Utilities

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Miscellaneous utility declarations.

*/

#ifndef _UTIL_H_
#define _UTIL_H_

#ifdef DEBUG
void
OutputDebugger(
    const char *funct,
    const char *format,
    ...
    );
#endif // DEBUG

#ifdef DEBUG
void
OutputFile(
    const char *funct,
    const char *format,
    ...
    );
#endif // DEBUG

#endif // _UTIL_H_
