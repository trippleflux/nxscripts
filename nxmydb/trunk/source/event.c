/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    Group File Backend

Author:
    neoxed (neoxed@gmail.com) Jun 5, 2006

Abstract:
    Entry point and functions for the event module.

*/

#include <base.h>
#include <database.h>

static BOOL EventHandler(EVENT_DATA *data, IO_STRING *arguments)
{
    return TRUE;
}

BOOL EventInit(EVENT_MODULE *module)
{
    return TRUE;
}
