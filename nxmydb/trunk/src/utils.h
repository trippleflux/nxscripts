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

#endif // _UTIL_H_
