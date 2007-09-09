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

TIMER *Io_StartIoTimer(TIMER *hTimer, Io_TimerProc *pTimerProc, VOID *pTimerContext, DWORD dwTimeOut);
BOOL   Io_StopIoTimer(TIMER *hTimer, BOOL bInTimerProc);
BOOL   Io_Putlog(DWORD dwLogCode, const CHAR *szFormatString, ...);


//
// Procedure table
//

typedef struct {
    CHAR  *(* ConfigGet)(CHAR *, CHAR *, CHAR *, INT *);
    BOOL   (* ConfigGetBool)(CHAR *, CHAR *, BOOL *);
    BOOL   (* ConfigGetInt)(CHAR *, CHAR *, INT *);
    CHAR  *(* ConfigGetPath)(CHAR *, CHAR *, CHAR *, CHAR *);

    INT32 *(* GetGroups)(DWORD *);
    CHAR  *(* Gid2Group)(INT32);
    INT32  (* Group2Gid)(CHAR *);
    BOOL   (* Ascii2GroupFile)(CHAR *, DWORD, GROUPFILE *);
    BOOL   (* GroupFile2Ascii)(BUFFER *, GROUPFILE *);

    INT32 *(* GetUsers)(DWORD *);
    CHAR  *(* Uid2User)(INT32);
    INT32  (* User2Uid)(CHAR *);
    BOOL   (* Ascii2UserFile)(CHAR *, DWORD, USERFILE *);
    BOOL   (* UserFile2Ascii)(BUFFER *, USERFILE *);

    VOID  *(* Allocate)(DWORD);
    VOID  *(* ReAllocate)(VOID *, DWORD);
    BOOL   (* Free)(VOID *);

    TIMER *(* StartIoTimer)(TIMER *, Io_TimerProc *, VOID *, DWORD);
    BOOL   (* StopIoTimer)(TIMER *, BOOL);
    BOOL   (* Putlog)(DWORD, const CHAR *, ...);
} PROC_TABLE;

extern PROC_TABLE procTable;

#define Io_ConfigGet        procTable.ConfigGet
#define Io_ConfigGetBool    procTable.ConfigGetBool
#define Io_ConfigGetInt     procTable.ConfigGetInt
#define Io_ConfigGetPath    procTable.ConfigGetPath

#define Io_GetGroups        procTable.GetGroups
#define Io_Gid2Group        procTable.Gid2Group
#define Io_Group2Gid        procTable.Group2Gid
#define Io_Ascii2GroupFile  procTable.Ascii2GroupFile
#define Io_GroupFile2Ascii  procTable.GroupFile2Ascii

#define Io_GetUsers         procTable.GetUsers
#define Io_Uid2User         procTable.Uid2User
#define Io_User2Uid         procTable.User2Uid
#define Io_Ascii2UserFile   procTable.Ascii2UserFile
#define Io_UserFile2Ascii   procTable.UserFile2Ascii

#define Io_Allocate         procTable.Allocate
#define Io_ReAllocate       procTable.ReAllocate
#define Io_Free             procTable.Free

#define Io_StartIoTimer     procTable.StartIoTimer
#define Io_StopIoTimer      procTable.StopIoTimer
#define Io_Putlog           procTable.Putlog


BOOL FCALL ProcTableInit(Io_GetProc *getProc);
VOID FCALL ProcTableFinalize(VOID);

#endif // PROCTABLE_H_INCLUDED
