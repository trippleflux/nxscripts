#ifndef __TIMES_H__
#define __TIMES_H__

BOOL  GetTimeZoneBias(LPLONG plBias);
ULONG FileTimeToPosixEpoch(const LPFILETIME pFileTime);
VOID  PosixEpochToFileTime(ULONG ulEpochTime, LPFILETIME pFileTime);

INT TclTimeCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[]);

#endif // __TIMES_H__
