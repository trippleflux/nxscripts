/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    Utilities

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Miscellaneous debugging utilities.

*/

#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

#ifdef DEBUG
VOID TraceHeader(VOID);
VOID TraceFormat(const char *funct, const char *format, ...);
VOID TraceFooter(VOID);
#endif // DEBUG

#endif // DEBUG_H_INCLUDED
