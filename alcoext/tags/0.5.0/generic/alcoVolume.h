/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    alcoVolume.h

Author:
    neoxed (neoxed@gmail.com) Jun 2, 2005

Abstract:
    Volume command definitions.

--*/

#ifndef _ALCOVOLUME_H_
#define _ALCOVOLUME_H_

typedef struct {
    Tcl_WideUInt free;      // Total number of free bytes.
    Tcl_WideUInt total;     // Total number of bytes.
    unsigned long flags;    // File system flags.
    unsigned long length;   // File system max component length.
    unsigned long id;       // Volume identification number.
    char name[128];         // Volume name.
    char type[64];          // File system type.
} VolumeInfo;

// Volume list flags.
#define VOLLIST_FLAG_LOCAL      0x0001  // Only list local volumes.
#define VOLLIST_FLAG_MOUNTS     0x0002  // Include mount points.
#define VOLLIST_FLAG_ROOT       0x0004  // Include volume roots.

int
GetVolumeInfo(
    Tcl_Interp *interp,
    char *volumePath,
    VolumeInfo *volumeInfo
    );

Tcl_Obj *
GetVolumeList(
    Tcl_Interp *interp,
    unsigned short options
    );

Tcl_ObjCmdProc VolumeObjCmd;

#endif // _ALCOVOLUME_H_
