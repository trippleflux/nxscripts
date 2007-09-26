/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    Event

Author:
    neoxed (neoxed@gmail.com) Sep 24, 2007

Abstract:
    Entry point and functions for the event module.

*/

#include <base.h>
#include <backends.h>
#include <database.h>

#pragma warning(disable : 4152) // C4152: nonstandard extension, function/data pointer conversion in expression

//
// Event structures
//

typedef DWORD (FCALL EVENT_HANDLER_PROC)(EVENT_DATA *data, IO_STRING *arguments);

typedef struct {
    CHAR                *name;  // Name of the event
    EVENT_HANDLER_PROC  *proc;  // Procedure to call for the event
} EVENT_HANDLER_TABLE;

//
// Event functions and variables
//

static EVENT_HANDLER_PROC EventHistory;
static EVENT_HANDLER_PROC EventStart;
static EVENT_HANDLER_PROC EventStop;

static const EVENT_HANDLER_TABLE eventTable[] = {
    {"HISTORY", EventHistory},
    {"START",   EventStart},
    {"STOP",    EventStop},
};


static DWORD FCALL EventHistory(EVENT_DATA *data, IO_STRING *arguments)
{
    ASSERT(data != NULL);
    ASSERT(arguments != NULL);
    TRACE("data=%p arguments=%p\n", data, arguments);

    // TODO

    return ERROR_SUCCESS;
}

static DWORD FCALL EventStart(EVENT_DATA *data, IO_STRING *arguments)
{
    UNREFERENCED_PARAMETER(data);
    UNREFERENCED_PARAMETER(arguments);
    TRACE("data=%p arguments=%p\n", data, arguments);

    DbSyncStart();

    return ERROR_SUCCESS;
}

static DWORD FCALL EventStop(EVENT_DATA *data, IO_STRING *arguments)
{
    UNREFERENCED_PARAMETER(data);
    UNREFERENCED_PARAMETER(arguments);
    TRACE("data=%p arguments=%p\n", data, arguments);

    DbSyncStop();

    return ERROR_SUCCESS;
}


static INT EventHandler(EVENT_DATA *data, IO_STRING *arguments)
{
    CHAR    *name;
    DWORD   i;
    DWORD   result;

    ASSERT(data != NULL);
    ASSERT(arguments != NULL);
    TRACE("data=%p arguments=%p\n", data, arguments);

    if (GetStringItems(arguments) < 1) {
        TRACE("No arguments passed to event handler.\n");
        return 1;
    }

    name = Io_GetStringIndexStatic(arguments, 0);

    // Look up the event name
    for (i = 0; i < ELEMENT_COUNT(eventTable); i++) {
        if (_stricmp(name, eventTable[i].name) != 0) {
            continue;
        }

        // Execute event procedure
        result = eventTable[i].proc(data, arguments);
        if (result != ERROR_SUCCESS) {
            SetLastError(result);
            return 1;
        }
        return 0;
    }

    TRACE("No event handler found for \"%s\".\n", name);
    return 1;
}

INT EventInit(EVENT_MODULE *module)
{
    INT failed;

    ASSERT(module != NULL);
    TRACE("module=%p\n", module);

    // Initialize module
    module->szName = MODULE_NAME;

    // Initialize database
    if (!DbInit(module->lpGetProc)) {
        TRACE("Unable to initialize module.\n");
        return 1;
    }

    // Register event handler
    failed = module->lpInstallEvent(MODULE_NAME, EventHandler);
    if (failed) {
        Io_Putlog(LOG_ERROR, "nxMyDB: Unable to register event handler.\n");

        DbFinalize();
        return 1;
    }

    return 0;
}

VOID EventDeInit(EVENT_MODULE *module)
{
    UNREFERENCED_PARAMETER(module);
    TRACE("module=%p\n", module);

    // Finalize database
    DbFinalize();
}
