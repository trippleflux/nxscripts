#ifndef __FREE_H__
#define __FREE_H__

typedef BOOL (WINAPI *PFNGETDISKFREESPACEEX)(LPCSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);

BOOL GetDiskSpace(LPCTSTR pszRootPath, PUINT64 i64FreeBytes, PUINT64 i64TotalBytes);
INT  TclFreeCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[]);

#endif // __FREE_H__
