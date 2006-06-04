/*

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    Procedure Table

Author:
    neoxed (neoxed@gmail.com) Jun 4, 2006

Abstract:
    Resolve procedures exported by ioFTPD.

*/

#ifndef _PROCS_H_
#define _PROCS_H_

//
// Procedure declarations
//

char *Io_ConfigGet(char *szArray, char *szVariable, char *szBuffer, int *pOffset);
BOOL  Io_ConfigGetBool(char *szArray, char *szVariable, BOOL *pValue);
BOOL  Io_ConfigGetInt(char *szArray, char *szVariable, int *pValue);
char *Io_ConfigGetPath(char *szArray, char *szVariable, char *szSuffix, char *szBuffer);

char *Io_Gid2Group(INT32 Gid);
INT32 Io_Group2Gid(char *szGroupName);

BOOL  Io_Ascii2GroupFile(char *pBuffer, DWORD dwBuffer, GROUPFILE *pGroupFile);
BOOL  Io_GroupFile2Ascii(BUFFER *pBuffer, GROUPFILE *pGroupFile);

char *Io_Uid2User(INT32 Uid);
INT32 Io_User2Uid(char *szUserName);

BOOL  Io_Ascii2UserFile(char *pBuffer, DWORD dwBuffer, USERFILE *pUserFile);
BOOL  Io_UserFile2Ascii(BUFFER *pBuffer, USERFILE *pUserFile);

void *Io_Allocate(DWORD Size);
void *Io_ReAllocate(void *pMemory, DWORD Size);
BOOL  Io_Free(void *pMemory);

void *Io_StartIoTimer(void *hTimer, void *pTimerProc, void *pTimerContext, DWORD dwTimeOut);
BOOL  Io_StopIoTimer(void *hTimer, BOOL bInTimerProc);
BOOL  Io_Putlog(DWORD dwLogCode, const char *szFormatString, ...);


//
// Procedure table
//

typedef struct {
    char *(* ConfigGet)(char *, char *, char *, int *);
    BOOL  (* ConfigGetBool)(char *, char *, BOOL *);
    BOOL  (* ConfigGetInt)(char *, char *, int *);
    char *(* ConfigGetPath)(char *, char *, char *, char *);
    char *(* Gid2Group)(INT32);
    INT32 (* Group2Gid)(char *);
    BOOL  (* Ascii2GroupFile)(char *, DWORD, GROUPFILE *);
    BOOL  (* GroupFile2Ascii)(BUFFER *, GROUPFILE *);
    char *(* Uid2User)(INT32);
    INT32 (* User2Uid)(char *);
    BOOL  (* Ascii2UserFile)(char *, DWORD, USERFILE *);
    BOOL  (* UserFile2Ascii)(BUFFER *, USERFILE *);
    void *(* Allocate)(DWORD);
    void *(* ReAllocate)(void *, DWORD);
    BOOL  (* Free)(void *);
    void *(* StartIoTimer)(void *, void *, void *, DWORD);
    BOOL  (* StopIoTimer)(void *, BOOL);
    BOOL  (* Putlog)(DWORD, const char *, ...);
} PROC_TABLE;

extern PROC_TABLE procTable;

#define Io_ConfigGet        procTable.ConfigGet
#define Io_ConfigGetBool    procTable.ConfigGetBool
#define Io_ConfigGetInt     procTable.ConfigGetInt
#define Io_ConfigGetPath    procTable.ConfigGetPath
#define Io_Gid2Group        procTable.Gid2Group
#define Io_Group2Gid        procTable.Group2Gid
#define Io_Ascii2GroupFile  procTable.Ascii2GroupFile
#define Io_GroupFile2Ascii  procTable.GroupFile2Ascii
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


typedef void *(Io_GetProc)(
    char *name
    );

BOOL
InitProcTable(
    Io_GetProc *getProc
    );

void
FinalizeProcTable(
    void
    );

#endif // _PROCS_H_
