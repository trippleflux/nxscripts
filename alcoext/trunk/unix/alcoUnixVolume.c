/*
 * AlcoExt - Alcoholicz Tcl extension.
 * Copyright (c) 2005 Alcoholicz Scripting Team
 *
 * File Name:
 *   alcoUnixVolume.c
 *
 * Author:
 *   neoxed (neoxed@gmail.com) June 2, 2005
 *
 * Abstract:
 *   Retrieve mount point information.
 */

#include <alcoExt.h>

const VolumeFlagList volumeFlags[] = {
    /* Flag names should be in alphabetical order. */
#ifdef MNT_ACLS
    {"acl",             MNT_ACLS},
#endif
#ifdef MNT_ASYNC
    {"async",           MNT_ASYNC},
#endif
#ifdef MNT_EXPORTANON
    {"expAnon",         MNT_EXPORTANON},
#endif
#ifdef MNT_DEFEXPORTED
    {"expDefault",      MNT_DEFEXPORTED},
#endif
#ifdef MNT_EXKERB
    {"expKerberos",     MNT_EXKERB},
#endif
#ifdef MNT_EXPUBLIC
    {"expPublic",       MNT_EXPUBLIC},
#endif
#ifdef MNT_EXRDONLY
    {"expReadOnly",     MNT_EXRDONLY},
#endif
#ifdef MNT_EXPORTED
    {"expReadWrite",    MNT_EXPORTED},
#endif
#ifdef MNT_IGNORE
    {"ignore",          MNT_IGNORE},
#endif
#ifdef MNT_LOCAL
    {"local",           MNT_LOCAL},
#endif
#ifdef MNT_MULTILABEL
    {"mac",             MNT_MULTILABEL},
#endif
#ifdef MS_MANDLOCK
    {"mandatoryLocks",  MS_MANDLOCK},
#endif
#ifdef MNT_NOATIME
    {"noAccessTime",    MNT_NOATIME},
#elif defined(MS_NOATIME)
    {"noAccessTime",    MS_NOATIME},
#endif
#ifdef MNT_NOCLUSTERR
    {"noClusterRead",   MNT_NOCLUSTERR},
#endif
#ifdef MNT_NOCLUSTERW
    {"noClusterWrite",  MNT_NOCLUSTERW},
#endif
#ifdef MNT_NODEV
    {"noDev",           MNT_NODEV},
#elif defined(MS_NODEV)
    {"noDev",           MS_NODEV},
#endif
#ifdef MS_NODIRATIME
    {"noDirAccessTime", MS_NODIRATIME},
#endif
#ifdef MNT_NOEXEC
    {"noExec",          MNT_NOEXEC},
#elif defined(MS_NOEXEC)
    {"noExec",          MS_NOEXEC},
#endif
#ifdef MNT_NOSUID
    {"noSuid",          MNT_NOSUID},
#elif defined(MS_NODEV)
    {"noSuid",          MS_NODEV},
#endif
#ifdef MNT_NOSYMFOLLOW
    {"noSymFollow",     MNT_NOSYMFOLLOW},
#endif
#ifdef MNT_QUOTA
    {"quotas",          MNT_QUOTA},
#endif
#ifdef MNT_RDONLY
    {"readOnly",        MNT_RDONLY},
#elif defined(MS_RDONLY)
    {"readOnly",        MS_RDONLY},
#endif
#ifdef MNT_ROOTFS
    {"rootFs",          MNT_ROOTFS},
#endif
#ifdef MNT_SOFTDEP
    {"softDep",         MNT_SOFTDEP},
#endif
#ifdef MNT_SUIDDIR
    {"suidDir",         MNT_SUIDDIR},
#endif
#ifdef MNT_SYNCHRONOUS
    {"synchronous",     MNT_SYNCHRONOUS},
#elif defined(MS_SYNCHRONOUS)
    {"synchronous",     MS_SYNCHRONOUS},
#endif
#ifdef MNT_UNION
    {"union",           MNT_UNION},
#endif
#ifdef MNT_USER
    {"userMounted",     MNT_USER},
#endif
    {NULL}
};


/*
 * GetVolumeInfo
 *
 *   Retrieves information about a volume.
 *
 * Arguments:
 *   interp     - Interpreter to use for error reporting.
 *   volumePath - A pointer to a string that specifies the volume.
 *   volumeInfo - A pointer to a VolumeInfo structure.
 *
 * Returns:
 *   A standard Tcl result.
 *
 * Remarks:
 *   None.
 */

int
GetVolumeInfo(Tcl_Interp *interp, char *volumePath, VolumeInfo *volumeInfo)
{
    struct STATFS_T fsInfo;

    if (STATFS_FN(volumePath, &fsInfo) != 0) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "unable to retrieve mount information for \"",
            volumePath, "\": ", Tcl_PosixError(interp), NULL);
        return TCL_ERROR;
    }

    /* Free and total space. */
    volumeInfo->free  = (uint64_t)fsInfo.f_bsize * (uint64_t)fsInfo.f_bfree;
    volumeInfo->total = (uint64_t)fsInfo.f_bsize * (uint64_t)fsInfo.f_blocks;

    volumeInfo->id     = (unsigned long) F_FSID(fsInfo);
    volumeInfo->flags  = (unsigned long) F_FLAGS(fsInfo);
    volumeInfo->length = (unsigned long) F_NAMELEN(fsInfo);

    /* Not supported. */
    volumeInfo->name[0] = '\0';

    /* File system name. */
#ifdef HAVE_STRUCT_STATFS_F_FSTYPENAME
    strncpy(volumeInfo->type, fsInfo.f_fstypename, VOLINFO_TYPE_LENGTH);
    volumeInfo->type[VOLINFO_TYPE_LENGTH-1] = '\0';
#else
    volumeInfo->type[0] = '\0';
#endif

    return TCL_OK;
}


/*
 * GetVolumeList
 *
 *   Retrieves a list of volumes and mount points.
 *
 * Arguments:
 *   interp  - Interpreter to use for error reporting.
 *   options - OR-ed value of flags that determine the returned volumes.
 *   pattern - Only volumes matching 'pattern' are returned. If this argument
 *             is NULL, all volumes are returned.
 *
 * Returns:
 *   A Tcl list object with applicable volumes and mount points. If the function
 *   fails, NULL is returned and an error message is left in the interpreter's result.
 *
 * Remarks:
 *   None.
 */

Tcl_Obj *
GetVolumeList(Tcl_Interp *interp, unsigned short options, char *pattern)
{
    Tcl_Obj *volumeList = Tcl_NewObj();

    /* TODO */

    return volumeList;
}
