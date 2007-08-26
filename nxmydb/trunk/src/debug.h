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

//
// Debugger output
//

#ifdef DEBUG
void
TraceDebugHeader(
    void
    );

void
TraceDebugFormat(
    const char *funct,
    const char *format,
    ...
    );

void
TraceDebugFooter(
    void
    );
#endif // DEBUG


//
// File output
//

#ifdef DEBUG
void
TraceFileHeader(
    void
    );

void
TraceFileFormat(
    const char *funct,
    const char *format,
    ...
    );

void
TraceFileFooter(
    void
    );
#endif // DEBUG

#endif // _DEBUG_H_
