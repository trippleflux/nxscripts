/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2009 neoxed

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

#ifdef DEBUG
static EVENT_HANDLER_PROC EventDebug;
#endif // DEBUG
static EVENT_HANDLER_PROC EventPurge;
static EVENT_HANDLER_PROC EventStart;
static EVENT_HANDLER_PROC EventStop;

static const EVENT_HANDLER_TABLE eventTable[] = {
#ifdef DEBUG
    {"DEBUG", EventDebug},
#endif // DEBUG
    {"PURGE", EventPurge},
    {"START", EventStart},
    {"STOP",  EventStop},
};


#ifdef DEBUG
static DWORD FCALL EventDebug(EVENT_DATA *data, IO_STRING *arguments)
{
    UNREFERENCED_PARAMETER(data);
    UNREFERENCED_PARAMETER(arguments);
    TRACE("data=%p arguments=%p", data, arguments);

    // TODO

    return ERROR_SUCCESS;
}
#endif // DEBUG

static DWORD FCALL EventPurge(EVENT_DATA *data, IO_STRING *arguments)
{
    UNREFERENCED_PARAMETER(data);
    UNREFERENCED_PARAMETER(arguments);
    TRACE("data=%p arguments=%p", data, arguments);

    DbSyncPurge();

    return ERROR_SUCCESS;
}

static DWORD FCALL EventStart(EVENT_DATA *data, IO_STRING *arguments)
{
    UNREFERENCED_PARAMETER(data);
    UNREFERENCED_PARAMETER(arguments);
    TRACE("data=%p arguments=%p", data, arguments);

    DbSyncStart();

    return ERROR_SUCCESS;
}

static DWORD FCALL EventStop(EVENT_DATA *data, IO_STRING *arguments)
{
    UNREFERENCED_PARAMETER(data);
    UNREFERENCED_PARAMETER(arguments);
    TRACE("data=%p arguments=%p", data, arguments);

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
    TRACE("data=%p arguments=%p", data, arguments);

    if (GetStringItems(arguments) < 1) {
        LOG_ERROR("No arguments passed to event handler.");
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

    LOG_ERROR("No event handler found for \"%s\".", name);
    return 1;
}

INT EventInit(EVENT_MODULE *module)
{
    INT failed;

    ASSERT(module != NULL);
    TRACE("module=%p", module);

    // Initialize module
    module->szName = MODULE_NAME;

    // Initialize database
    if (!DbInit(module->lpGetProc)) {
        TRACE("Unable to initialize module.");
        return 1;
    }

    // Register event handler
    failed = module->lpInstallEvent(MODULE_NAME, EventHandler);
    if (failed) {
        LOG_ERROR("Unable to register event handler (error %lu).", GetLastError());

        DbFinalize();
        return 1;
    }

    return 0;
}

VOID EventDeInit(EVENT_MODULE *module)
{
    UNREFERENCED_PARAMETER(module);
    TRACE("module=%p", module);

    // Finalize database
    DbFinalize();
}
