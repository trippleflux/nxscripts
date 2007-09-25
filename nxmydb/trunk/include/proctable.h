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

typedef VOID *(Io_GetProc)(CHAR *szName);
typedef DWORD (Io_TimerProc)(VOID *pTimerContext, TIMER *hTimer);

//
// Function declarations
//

CHAR  *Io_ConfigGet(CHAR *szArray, CHAR *szVariable, CHAR *szBuffer, INT *pOffset);
BOOL   Io_ConfigGetBool(CHAR *szArray, CHAR *szVariable, BOOL *pValue);
BOOL   Io_ConfigGetInt(CHAR *szArray, CHAR *szVariable, INT *pValue);
CHAR  *Io_ConfigGetPath(CHAR *szArray, CHAR *szVariable, CHAR *szSuffix, CHAR *szBuffer);

INT32 *Io_GetGroups(DWORD *plpGroupIdCount);
CHAR  *Io_Gid2Group(INT32 Gid);
INT32  Io_Group2Gid(CHAR *szGroupName);
BOOL   Io_Ascii2GroupFile(CHAR *pBuffer, DWORD dwBuffer, GROUPFILE *pGroupFile);
BOOL   Io_GroupFile2Ascii(BUFFER *pBuffer, GROUPFILE *pGroupFile);

INT32 *Io_GetUsers(DWORD *pUserIdCount);
CHAR  *Io_Uid2User(INT32 Uid);
INT32  Io_User2Uid(CHAR *szUserName);
BOOL   Io_Ascii2UserFile(CHAR *pBuffer, DWORD dwBuffer, USERFILE *pUserFile);
BOOL   Io_UserFile2Ascii(BUFFER *pBuffer, USERFILE *pUserFile);

VOID  *Io_Allocate(DWORD Size);
VOID  *Io_ReAllocate(VOID *pMemory, DWORD Size);
BOOL   Io_Free(VOID *pMemory);

CHAR  *Io_GetStringIndex(IO_STRING *String, DWORD Index);
CHAR  *Io_GetStringIndexStatic(IO_STRING *String, DWORD Index);
CHAR  *Io_GetStringRange(IO_STRING *String, DWORD BeginIndex, DWORD EndIndex);
VOID   Io_FreeString(IO_STRING *String);

TIMER *Io_StartIoTimer(TIMER *hTimer, Io_TimerProc *pTimerProc, VOID *pTimerContext, DWORD dwTimeOut);
BOOL   Io_StopIoTimer(TIMER *hTimer, BOOL bInTimerProc);
BOOL   Io_Putlog(DWORD dwLogCode, const CHAR *szFormatString, ...);


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

    TIMER *(* pStartIoTimer)(TIMER *, Io_TimerProc *, VOID *, DWORD);
    BOOL   (* pStopIoTimer)(TIMER *, BOOL);
    BOOL   (* pPutlog)(DWORD, const CHAR *, ...);
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

#define Io_StartIoTimer         procTable.pStartIoTimer
#define Io_StopIoTimer          procTable.pStopIoTimer
#define Io_Putlog               procTable.pPutlog


BOOL FCALL ProcTableInit(Io_GetProc *getProc);
VOID FCALL ProcTableFinalize(VOID);

#endif // PROCTABLE_H_INCLUDED
