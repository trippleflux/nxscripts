/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005 Alcoholicz Scripting Team

Module Name:
    alcoWinUtil.h

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
    GetDiskFreeSpaceExProc                  getDiskFreeSpaceEx;
    FindFirstVolumeMountPointProc           findFirstVolumeMountPoint;
    FindNextVolumeMountPointProc            findNextVolumeMountPoint;
    FindVolumeMountPointCloseProc           findVolumeMountPointClose;
    GetVolumeNameForVolumeMountPointProc    getVolumeNameForVolumeMountPoint;
} WinProcs;


// Flags for IsFeatureAvailable().
#define FEATURE_DISKSPACEEX     0x00000001
#define FEATURE_MOUNT_POINTS    0x00000002

int
IsFeatureAvailable(
    unsigned long features
    );

char *
TclSetWinError(
    Tcl_Interp *interp,
    unsigned long errorCode
    );

#endif // _ALCOWINUTIL_H_
