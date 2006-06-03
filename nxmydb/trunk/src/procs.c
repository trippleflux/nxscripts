/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Procedures

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Resolve procedures exported by ioFTPD.

*/

#include "mydb.h"

IO_PROC_TABLE procTable;

#define RESOLVE(name, func)           \
    func = getProc(name);             \
    if (func == NULL) { goto error; }


BOOL
InitProcs(
    GetProc *getProc
    )
{
    ZeroMemory(&procTable, sizeof(IO_PROC_TABLE));

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
    ZeroMemory(&procTable, sizeof(IO_PROC_TABLE));
    return FALSE;
}

void
FinalizeProcs(
    void
    )
{
}
