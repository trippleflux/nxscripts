/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    alcoUnixVolume.c

Author:
    neoxed (neoxed@gmail.com) Jun 2, 2005

Abstract:
    Retrieve mount point information.

--*/

#include <alcoExt.h>

/*++

GetVolumeInfo

    Retrieves information about a volume.

Arguments:
    interp     - Interpreter to use for error reporting.

    volumePath - A pointer to a string that specifies the volume.

    volumeInfo - A pointer to a "VolumeInfo" structure.

Return Value:
    A standard Tcl result.

--*/
int
GetVolumeInfo(
    Tcl_Interp *interp,
    char *volumePath,
    VolumeInfo *volumeInfo
    )
{
    struct STATFS_T fsInfo;

    if (STATFS_FN(volumePath, &fsInfo) != 0) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "unable to retrieve mount information for \"",
            volumePath, "\": ", Tcl_PosixError(interp), NULL);
        return TCL_ERROR;
    }

    // Free and total space.
    volumeInfo->free  = (Tcl_WideUInt)fsInfo.f_bsize * (Tcl_WideUInt)fsInfo.f_bfree;
    volumeInfo->total = (Tcl_WideUInt)fsInfo.f_bsize * (Tcl_WideUInt)fsInfo.f_blocks;

    volumeInfo->id     = (unsigned long)F_FSID(fsInfo);
    volumeInfo->flags  = (unsigned long)F_FLAGS(fsInfo);
    volumeInfo->length = (unsigned long)F_NAMELEN(fsInfo);

    // Not supported.
    volumeInfo->name[0] = '\0';

    // File system type.
    strncpy(volumeInfo->type, F_TYPENAME(fsInfo), ARRAYSIZE(volumeInfo->type));
    volumeInfo->type[ARRAYSIZE(volumeInfo->type)-1] = '\0';

    return TCL_OK;
}

/*++

GetVolumeList

    Retrieves a list of volumes and mount points.

Arguments:
    interp  - Interpreter to use for error reporting.

    options - OR-ed value of flags that determine the returned volumes.

Return Value:
    A Tcl list object with applicable volumes and mount points. If the
    function fails, NULL is returned and an error message is left in the
    interpreter's result.

--*/
Tcl_Obj *
GetVolumeList(
    Tcl_Interp *interp,
    unsigned short options
    )
{
    Tcl_Obj *volumeList = Tcl_NewObj();

    // The only "root" path on a UNIX system is "/".
    if (options & VOLLIST_FLAG_ROOT) {
        Tcl_ListObjAppendElement(NULL, volumeList, Tcl_NewStringObj("/", 1));
    }

    // Append all mount points.
#ifdef HAVE_GETMNTINFO
    if (options & VOLLIST_FLAG_MOUNTS) {
        int count;
        int i;
        struct statfs *mounts;

        count = getmntinfo(&mounts, MNT_NOWAIT);
        if (count == 0) {
            Tcl_ResetResult(interp);
            Tcl_AppendResult(interp, "unable to retrieve mount points: ",
                Tcl_PosixError(interp), NULL);

            Tcl_DecrRefCount(volumeList);
            return NULL;
        }

        for (i = 0; i < count; i++) {
            if ((options & VOLLIST_FLAG_LOCAL) && !(mounts[i].f_flags & MNT_LOCAL)) {
                continue;
            }

            Tcl_ListObjAppendElement(NULL, volumeList,
                Tcl_NewStringObj(mounts[i].f_mntonname, -1));
        }
    }
#endif // HAVE_GETMNTINFO

    return volumeList;
}
