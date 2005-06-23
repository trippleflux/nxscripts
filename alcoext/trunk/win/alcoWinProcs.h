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

typedef BOOL (WINAPI *Fn_GetDiskFreeSpaceExA)(
    LPCSTR lpDirectoryName,
    PULARGE_INTEGER lpFreeBytesAvailableToCaller,
    PULARGE_INTEGER lpTotalNumberOfBytes,
    PULARGE_INTEGER lpTotalNumberOfFreeBytes
);

typedef HANDLE (WINAPI *Fn_FindFirstVolumeMountPointA)(
    LPCSTR lpszRootPathName,
    LPSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
);

typedef BOOL (WINAPI *Fn_FindNextVolumeMountPointA)(
    HANDLE hFindVolumeMountPoint,
    LPSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
);

typedef BOOL (WINAPI *Fn_FindVolumeMountPointClose)(
    HANDLE hFindVolumeMountPoint
);

typedef BOOL (WINAPI *Fn_GetVolumeNameForVolumeMountPointA)(
    LPCSTR lpszVolumeMountPoint,
    LPTSTR lpszVolumeName,
    DWORD cchBufferLength
);

typedef struct {
    HMODULE kernelModule;
    Fn_GetDiskFreeSpaceExA        getDiskFreeSpaceEx;
    Fn_FindFirstVolumeMountPointA findFirstVolumeMountPoint;
    Fn_FindNextVolumeMountPointA  findNextVolumeMountPoint;
    Fn_FindVolumeMountPointClose  findVolumeMountPointClose;
    Fn_GetVolumeNameForVolumeMountPointA getVolumeNameForVolumeMountPoint;
} WinProcs;

#endif /* __ALCOWINPROCS_H__ */
