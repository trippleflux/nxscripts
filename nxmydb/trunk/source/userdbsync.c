/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    User Database Sync

Author:
    neoxed (neoxed@gmail.com) Sep 9, 2007

Abstract:
    User database synchronization.

*/

#include <base.h>
#include <backends.h>
#include <database.h>

DWORD DbUserSync(DB_CONTEXT *db, SYNC_CONTEXT *sync)
{
    ASSERT(db != NULL);
    ASSERT(sync != NULL);
    TRACE("db=%p sync=%p\n", db, sync);

    // TODO

    return ERROR_INTERNAL_ERROR;
}
