/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Windows Utilities

Author:
    neoxed (neoxed@gmail.com) May 6, 2005

Abstract:
    Miscellaneous Windows specific utilities.

--*/

#ifndef _ALCOWINUTIL_H_
#define _ALCOWINUTIL_H_

//
// Declarations for Windows API functions resolved on run-time.
//

typedef BOOL (WINAPI *GetDiskFreeSpaceExProc)(
    LPCSTR lpDirectoryName,
    PULARGE_INTEGER lpFreeBytesAvailableToCaller,
    PULARGE_INTEGER lpTotalNumberOfBytes,
    PULARGE_INTEGER lpTotalNumberOfFreeBytes
    );

typedef HANDLE (WINAPI *FindFirstVolumeMountPointProc)(
    LPCSTR lpszRootPathName,
    LPSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    );

typedef BOOL (WINAPI *FindNextVolumeMountPointProc)(
    HANDLE hFindVolumeMountPoint,
    LPSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    );

typedef BOOL (WINAPI *GetVolumeNameForVolumeMountPointProc)(
    LPCSTR lpszVolumeMountPoint,
    LPSTR lpszVolumeName,
    DWORD cchBufferLength
    );

typedef BOOL (WINAPI *FindVolumeMountPointCloseProc)(
    HANDLE hFindVolumeMountPoint
    );

typedef struct {
    HMODULE                              module; // Handle to kernel32.dll
    GetDiskFreeSpaceExProc               getDiskFreeSpaceEx;
    FindFirstVolumeMountPointProc        findFirstVolumeMountPoint;
    FindNextVolumeMountPointProc         findNextVolumeMountPoint;
    FindVolumeMountPointCloseProc        findVolumeMountPointClose;
    GetVolumeNameForVolumeMountPointProc getVolumeNameForVolumeMountPoint;
} WinProcs;


//
// Time conversion functions.
//

void
EpochToFileTime(
    long epochTime,
    FILETIME *fileTime
    );

long
FileTimeToEpoch(
    const FILETIME *fileTime
    );


//
// Tcl wrapper functions.
//

char *
TclSetWinError(
    Tcl_Interp *interp,
    unsigned long errorCode
    );

int
TclGetOctalFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,
    unsigned long *octalPtr
    );

Tcl_Obj *
TclNewOctalObj(
    unsigned long octal
    );

void
TclSetOctalObj(
    Tcl_Obj *objPtr,
    unsigned long octal
    );

#endif // _ALCOWINUTIL_H_
