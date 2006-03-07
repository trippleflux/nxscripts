/*
 * nxHelper - Tcl extension for nxTools.
 * Copyright (c) 2005 neoxed
 *
 * File Name:
 *   nxVolume.h
 *
 * Author:
 *   neoxed (neoxed@gmail.com) May 22, 2005
 *
 * Abstract:
 *   Volume command definitions.
 */

#ifndef _NXVOLUME_H_
#define _NXVOLUME_H_

/* Buffer size constants. */
#define VOLUME_NAME_BUFFER  MAX_PATH
#define VOLUME_FS_BUFFER    128

typedef struct {
    ULONG     serial;
    ULONG     length;
    ULONG     flags;
    UINT      type;
    ULONGLONG bytesFree;
    ULONGLONG bytesTotal;
    TCHAR     fs[VOLUME_FS_BUFFER];
    TCHAR     name[VOLUME_NAME_BUFFER];
} VolumeInfo;

int VolumeObjCmd(ClientData dummy, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

#endif /* _NXVOLUME_H_ */
