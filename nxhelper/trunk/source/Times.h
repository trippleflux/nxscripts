#ifndef __TIMES_H__
#define __TIMES_H__

BOOL  GetTimeZoneBias(PLONG Bias);
ULONG FileTimeToPosixEpoch(const PFILETIME FileTime);
VOID  PosixEpochToFileTime(ULONG EpochTime, PFILETIME FileTime);

INT TclTimeCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[]);

#endif // __TIMES_H__
