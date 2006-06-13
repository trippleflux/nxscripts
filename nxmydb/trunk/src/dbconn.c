/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Database Connection

Author:
    neoxed (neoxed@gmail.com) Jun 13, 2006

Abstract:
    Database connection and initialization functions.

*/

#include "mydb.h"

/*++

DbInit

    Initializes the procedure table and database connection pool.

Arguments:
    getProc - Pointer to ioFTPD's GetProc function.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

Remarks:
    This function must be called once by each entry point (GroupModuleInit and UserModuleInit).

--*/
BOOL
DbInit(
    Io_GetProc *getProc
    )
{
    DebugPrint("DbInit", "getProc=%p\n", getProc);

    // Initialize procedure table
    if (!ProcTableInit(getProc)) {
        DebugPrint("DbInit", "Unable to initialize procedure table.\n");
        return FALSE;
    }

    Io_Putlog(LOG_ERROR, "nxMyDB: Module v%s loaded.\r\n", STRINGIFY(VERSION));
    return TRUE;
}

/*++

DbFinalize

    Finalizes the procedure table and database connection pool.

Arguments:
    None.

Return Values:
    None.

Remarks:
    This function must be called once by each module exit point.

--*/
void
DbFinalize(
    void
    )
{
    DebugPrint("DbFinalize", "\n");
    Io_Putlog(LOG_ERROR, "nxMyDB: Module v%s unloaded.\r\n", STRINGIFY(VERSION));

    ProcTableFinalize();
}
