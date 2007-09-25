/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    Events

Author:
    neoxed (neoxed@gmail.com) Sep 24, 2007

Abstract:
    Entry point and functions for the event module.

*/

#include <base.h>
#include <database.h>
#include <events.h>

static BOOL EventHandler(EVENT_DATA *data, IO_STRING *arguments)
{
    return TRUE;
}

BOOL EventInit(EVENT_MODULE *module)
{
    return TRUE;
}
