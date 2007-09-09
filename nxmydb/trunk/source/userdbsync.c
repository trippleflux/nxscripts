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

DWORD DbUserRefresh(DB_CONTEXT *db, ULONG lastUpdate)
{
    ASSERT(db != NULL);
    ASSERT(lastUpdate > 0);
    TRACE("db=%p lastUpdate=%lu\n", db, lastUpdate);

    // TODO

    return ERROR_INTERNAL_ERROR;
}
