#ifndef __VOLUME_H__
#define __VOLUME_H__

typedef BOOL (WINAPI *FNGETDISKFREESPACEEX)(LPCTSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);

BOOL GetDiskSpace(PTCHAR RootPath, PUINT64 FreeBytes, PUINT64 TotalBytes);
INT  TclVolumeCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[]);

#endif // __VOLUME_H__
