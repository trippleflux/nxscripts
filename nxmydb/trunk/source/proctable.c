/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    Procedure Table

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Resolve procedures exported by ioFTPD.

*/

#include <base.h>
#include <proctable.h>

// Global procedure table
PROC_TABLE procTable;

#pragma warning(disable : 4152) // C4152: nonstandard extension, function/data pointer conversion in expression

#define RESOLVE(name, func)                                                     \
{                                                                               \
    if ((func = getProc(name)) == NULL) {                                       \
        TRACE("Unable to resolve procedure \"%s\".\n", name);                   \
        goto failed;                                                            \
    }                                                                           \
}


/*++

ProcTableInit

    Initializes the procedure table.

Arguments:
    getProc - Pointer to ioFTPD's GetProc function.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
BOOL FCALL ProcTableInit(Io_GetProc *getProc)
{
    RESOLVE("Config_Get",      procTable.pConfigGet)
    RESOLVE("Config_GetBool",  procTable.pConfigGetBool)
    RESOLVE("Config_GetInt",   procTable.pConfigGetInt)
    RESOLVE("Config_GetPath",  procTable.pConfigGetPath)

    RESOLVE("GetGroups",       procTable.pGetGroups)
    RESOLVE("Gid2Group",       procTable.pGid2Group)
    RESOLVE("Group2Gid",       procTable.pGroup2Gid)
    RESOLVE("Ascii2GroupFile", procTable.pAscii2GroupFile)
    RESOLVE("GroupFile2Ascii", procTable.pGroupFile2Ascii)

    RESOLVE("GetUsers",        procTable.pGetUsers)
    RESOLVE("Uid2User",        procTable.pUid2User)
    RESOLVE("User2Uid",        procTable.pUser2Uid)
    RESOLVE("Ascii2UserFile",  procTable.pAscii2UserFile)
    RESOLVE("UserFile2Ascii",  procTable.pUserFile2Ascii)

    RESOLVE("Allocate",        procTable.pAllocate)
    RESOLVE("ReAllocate",      procTable.pReAllocate)
    RESOLVE("Free",            procTable.pFree)

    RESOLVE("StartIoTimer",    procTable.pStartIoTimer)
    RESOLVE("StopIoTimer",     procTable.pStopIoTimer)
    RESOLVE("Putlog",          procTable.pPutlog)
    return TRUE;

failed:
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
VOID FCALL ProcTableFinalize(VOID)
{
    ZeroMemory(&procTable, sizeof(PROC_TABLE));
}
