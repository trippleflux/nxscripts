/*
 * AlcoExt - Alcoholicz Tcl extension.
 * Copyright (c) 2005 Alcoholicz Scripting Team
 *
 * File Name:
 *   alcoVolume.h
 *
 * Author:
 *   neoxed (neoxed@gmail.com) June 2, 2005
 *
 * Abstract:
 *   Structures and definitions for volume related procedures.
 */

#ifndef _ALCOVOLUME_H_
#define _ALCOVOLUME_H_

/* Length of the 'VolumeInfo::name' member. */
#define VOLINFO_NAME_LENGTH     128

/* Length of the 'VolumeInfo::type' member. */
#define VOLINFO_TYPE_LENGTH     64

typedef struct {
    char *name;             /* Flag's name. */
    unsigned long flag;     /* Bit used for comparison. */
} VolumeFlagList;

typedef struct {
#ifdef _WINDOWS
    ULONGLONG free;         /* Total number of free bytes. */
    ULONGLONG total;        /* Total number of bytes. */
#else /* _WINDOWS */
    uint64_t free;          /* Total number of free bytes. */
    uint64_t total;         /* Total number of bytes. */
#endif /* _WINDOWS */

    unsigned long flags;    /* File system flags. */
    unsigned long length;   /* File system max component length. */
    unsigned long id;       /* Volume identification number. */

    char name[VOLINFO_NAME_LENGTH]; /* Volume name. */
    char type[VOLINFO_TYPE_LENGTH]; /* File system type. */
} VolumeInfo;

/* Volume list flags. */
#define VOLLIST_FLAG_LOCAL      0x0001  /* Only list local volumes. */
#define VOLLIST_FLAG_MOUNTS     0x0002  /* Include mount points. */

const VolumeFlagList volumeFlags[];
int GetVolumeInfo(Tcl_Interp *interp, char *volumePath, VolumeInfo *volumeInfo);
Tcl_Obj *GetVolumeList(Tcl_Interp *interp, unsigned short options, char *pattern);
Tcl_ObjCmdProc VolumeObjCmd;

#endif /* _ALCOVOLUME_H_ */
