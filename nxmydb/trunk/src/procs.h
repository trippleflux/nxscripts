/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Procedures

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Resolve procedures exported by ioFTPD.

*/

#ifndef _PROCS_H_
#define _PROCS_H_

typedef void * (GetProc)(
    char *name
    );

BOOL
InitProcs(
    GetProc *getProc
    );

void
FinalizeProcs(
    void
    );

#endif // _PROCS_H_
