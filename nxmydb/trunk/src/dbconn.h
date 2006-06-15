/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Database Connection

Author:
    neoxed (neoxed@gmail.com) Jun 13, 2006

Abstract:
    Database connection and initialization declarations.

*/

#ifndef _DBCONN_H_
#define _DBCONN_H_

BOOL
DbInit(
    Io_GetProc *getProc
    );

void
DbFinalize(
    void
    );

BOOL
DbAcquire(
    MYSQL **handle
    );

void
DbRelease(
    MYSQL *handle
    );

#endif // _DBCONN_H_