/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Procedure Table

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Resolve procedures exported by ioFTPD.

*/

#include "mydb.h"

// Silence C4152: nonstandard extension, function/data pointer conversion in expression
#pragma warning(disable : 4152)

// Global procedure table
PROC_TABLE procTable;

// Resolve functions
#define RESOLVE(name, func) if ((func = getProc(name)) == NULL) goto error;


/*++

ProcTableInit

    Initializes the procedure table.

Arguments:
    getProc - Pointer to ioFTPD's GetProc function.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
BOOL
ProcTableInit(
    Io_GetProc *getProc
    )
{
    DebugPrint("ProcTableInit", "getProc=%p\n", getProc);

    RESOLVE("Config_Get",      procTable.ConfigGet)
    RESOLVE("Config_GetBool",  procTable.ConfigGetBool)
    RESOLVE("Config_GetInt",   procTable.ConfigGetInt)
    RESOLVE("Config_GetPath",  procTable.ConfigGetPath)
    RESOLVE("Gid2Group",       procTable.Gid2Group)
    RESOLVE("Group2Gid",       procTable.Group2Gid)
    RESOLVE("Ascii2GroupFile", procTable.Ascii2GroupFile)
    RESOLVE("GroupFile2Ascii", procTable.GroupFile2Ascii)
    RESOLVE("Uid2User",        procTable.Uid2User)
    RESOLVE("User2Uid",        procTable.User2Uid)
    RESOLVE("Ascii2UserFile",  procTable.Ascii2UserFile)
    RESOLVE("UserFile2Ascii",  procTable.UserFile2Ascii)
    RESOLVE("Allocate",        procTable.Allocate)
    RESOLVE("ReAllocate",      procTable.ReAllocate)
    RESOLVE("Free",            procTable.Free)
    RESOLVE("StartIoTimer",    procTable.StartIoTimer)
    RESOLVE("StopIoTimer",     procTable.StopIoTimer)
    RESOLVE("Putlog",          procTable.Putlog)
    return TRUE;

error:
    // Unable to resolve a procedure
    ZeroMemory(&procTable, sizeof(PROC_TABLE));
    return FALSE;
}

/*++

ProcTableFinalize

    Finalizes the procedure table.

Arguments:
    None.

Return Values:
    None.

--*/
void
ProcTableFinalize(
    void
    )
{
    DebugPrint("ProcTableFinalize", "\n");
    ZeroMemory(&procTable, sizeof(PROC_TABLE));
}