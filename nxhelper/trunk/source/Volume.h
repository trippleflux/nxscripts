#ifndef __VOLUME_H__
#define __VOLUME_H__

typedef BOOL (WINAPI *PFNGETDISKFREESPACEEX)(LPCTSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);

BOOL GetDiskSpace(LPCTSTR pszRootPath, PUINT64 i64FreeBytes, PUINT64 i64TotalBytes);
INT  TclVolumeCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[]);

#endif // __VOLUME_H__
