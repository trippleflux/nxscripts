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

#ifndef PROCTABLE_H_INCLUDED
#define PROCTABLE_H_INCLUDED

//
// Callback declarations
//

typedef VOID *(CCALL Io_GetProc)(CHAR *Name);
typedef VOID  (CCALL Io_JobProc)(VOID *Context);
typedef DWORD (CCALL Io_TimerProc)(VOID *Context, TIMER *Timer);

//
// Function declarations
//

CHAR  *Io_ConfigGet(CHAR *Array, CHAR *Variable, CHAR *Buffer, INT *Offset);
BOOL   Io_ConfigGetBool(CHAR *Array, CHAR *Variable, BOOL *Value);
BOOL   Io_ConfigGetInt(CHAR *Array, CHAR *Variable, INT *Value);
CHAR  *Io_ConfigGetPath(CHAR *Array, CHAR *Variable, CHAR *Suffix, CHAR *Buffer);

INT32 *Io_GetGroups(DWORD *GroupIdCount);
CHAR  *Io_Gid2Group(INT32 Gid);
INT32  Io_Group2Gid(CHAR *GroupName);
BOOL   Io_Ascii2GroupFile(CHAR *Buffer, DWORD BufferSize, GROUPFILE *GroupFile);
BOOL   Io_GroupFile2Ascii(BUFFER *Buffer, GROUPFILE *GroupFile);

INT32 *Io_GetUsers(DWORD *UserIdCount);
CHAR  *Io_Uid2User(INT32 Uid);
INT32  Io_User2Uid(CHAR *UserName);
BOOL   Io_Ascii2UserFile(CHAR *Buffer, DWORD BufferSize, USERFILE *UserFile);
BOOL   Io_UserFile2Ascii(BUFFER *Buffer, USERFILE *UserFile);

VOID  *Io_Allocate(DWORD Size);
VOID  *Io_ReAllocate(VOID *Memory, DWORD Size);
BOOL   Io_Free(VOID *Memory);

CHAR  *Io_GetStringIndex(IO_STRING *String, DWORD Index);
CHAR  *Io_GetStringIndexStatic(IO_STRING *String, DWORD Index);
CHAR  *Io_GetStringRange(IO_STRING *String, DWORD BeginIndex, DWORD EndIndex);
VOID   Io_FreeString(IO_STRING *String);

BOOL   Io_QueueJob(Io_JobProc *Proc, VOID *Context, DWORD Priority);
BOOL   Io_Putlog(DWORD LogCode, const CHAR *Format, ...);
TIMER *Io_StartIoTimer(TIMER *Timer, Io_TimerProc *Proc, VOID *Context, DWORD Timeout);
BOOL   Io_StopIoTimer(TIMER *Timer, BOOL InTimerProc);


//
// Procedure table
//

typedef struct {
    CHAR  *(* pConfigGet)(CHAR *, CHAR *, CHAR *, INT *);
    BOOL   (* pConfigGetBool)(CHAR *, CHAR *, BOOL *);
    BOOL   (* pConfigGetInt)(CHAR *, CHAR *, INT *);
    CHAR  *(* pConfigGetPath)(CHAR *, CHAR *, CHAR *, CHAR *);

    INT32 *(* pGetGroups)(DWORD *);
    CHAR  *(* pGid2Group)(INT32);
    INT32  (* pGroup2Gid)(CHAR *);
    BOOL   (* pAscii2GroupFile)(CHAR *, DWORD, GROUPFILE *);
    BOOL   (* pGroupFile2Ascii)(BUFFER *, GROUPFILE *);

    INT32 *(* pGetUsers)(DWORD *);
    CHAR  *(* pUid2User)(INT32);
    INT32  (* pUser2Uid)(CHAR *);
    BOOL   (* pAscii2UserFile)(CHAR *, DWORD, USERFILE *);
    BOOL   (* pUserFile2Ascii)(BUFFER *, USERFILE *);

    VOID  *(* pAllocate)(DWORD);
    VOID  *(* pReAllocate)(VOID *, DWORD);
    BOOL   (* pFree)(VOID *);

    CHAR  *(* pGetStringIndex)(IO_STRING *, DWORD);
    CHAR  *(* pGetStringIndexStatic)(IO_STRING *, DWORD);
    CHAR  *(* pGetStringRange)(IO_STRING *, DWORD, DWORD);
    VOID   (* pFreeString)(IO_STRING *);

    BOOL   (* pPutlog)(DWORD, const CHAR *, ...);
    BOOL   (* pQueueJob)(Io_JobProc *, VOID *, DWORD);
    TIMER *(* pStartIoTimer)(TIMER *, Io_TimerProc *, VOID *, DWORD);
    BOOL   (* pStopIoTimer)(TIMER *, BOOL);
} PROC_TABLE;

extern PROC_TABLE procTable;

#define Io_ConfigGet            procTable.pConfigGet
#define Io_ConfigGetBool        procTable.pConfigGetBool
#define Io_ConfigGetInt         procTable.pConfigGetInt
#define Io_ConfigGetPath        procTable.pConfigGetPath

#define Io_GetGroups            procTable.pGetGroups
#define Io_Gid2Group            procTable.pGid2Group
#define Io_Group2Gid            procTable.pGroup2Gid
#define Io_Ascii2GroupFile      procTable.pAscii2GroupFile
#define Io_GroupFile2Ascii      procTable.pGroupFile2Ascii

#define Io_GetUsers             procTable.pGetUsers
#define Io_Uid2User             procTable.pUid2User
#define Io_User2Uid             procTable.pUser2Uid
#define Io_Ascii2UserFile       procTable.pAscii2UserFile
#define Io_UserFile2Ascii       procTable.pUserFile2Ascii

#define Io_Allocate             procTable.pAllocate
#define Io_ReAllocate           procTable.pReAllocate
#define Io_Free                 procTable.pFree

#define Io_GetStringIndex       procTable.pGetStringIndex
#define Io_GetStringIndexStatic procTable.pGetStringIndexStatic
#define Io_GetStringRange       procTable.pGetStringRange
#define Io_FreeString           procTable.pFreeString

#define Io_Putlog               procTable.pPutlog
#define Io_QueueJob             procTable.pQueueJob
#define Io_StartIoTimer         procTable.pStartIoTimer
#define Io_StopIoTimer          procTable.pStopIoTimer


DWORD FCALL ProcTableInit(Io_GetProc *getProc);
DWORD FCALL ProcTableFinalize(VOID);

#endif // PROCTABLE_H_INCLUDED
