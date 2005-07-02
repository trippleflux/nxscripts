/*
 * AlcoExt - Alcoholicz Tcl extension.
 * Copyright (c) 2005 Alcoholicz Scripting Team
 *
 * File Name:
 *   alcoWinProcs.h
 *
 * Author:
 *   neoxed (neoxed@gmail.com) May 6, 2005
 *
 * Abstract:
 *   Declarations for Windows API functions resolved on run-time.
 */

#ifndef __ALCOWINPROCS_H__
#define __ALCOWINPROCS_H__

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

#endif /* __ALCOWINPROCS_H__ */
