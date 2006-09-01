/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Volumes

Author:
    neoxed (neoxed@gmail.com) Jun 2, 2005

Abstract:
    This module implements a interface to list and query volumes.

    volume list [-local] [-mounts] [-root]
      - Retrieves a list of volumes and mount points.

    volume info <volume> <varName>
     - Retrieves information for the given volume, and uses the
       variable given by varName to store the returned information.
     - Array Contents:
       flags  - A list of lags associated with the file system.
       id     - Volume identification number.
       length - Maximum length of a file name.
       name   - Name of the volume.
       type   - Volume type and file system name.
       free   - Available space, expressed in bytes.
       used   - Used space, expressed in bytes.
       total  - Total space, expressed in bytes.

--*/

#include <alcoExt.h>

static const struct {
    char *name;         // Flag's name.
    unsigned long flag; // Bit used for comparison.
} volumeFlags[] = {
#ifdef _WINDOWS
    {"acl",             FS_PERSISTENT_ACLS},
    {"caseIsPreserved", FS_CASE_IS_PRESERVED},
    {"caseSensitive",   FS_CASE_SENSITIVE},
    {"compressed",      FS_VOL_IS_COMPRESSED},
    {"fileCompression", FS_FILE_COMPRESSION},
    {"fileEncryption",  FS_FILE_ENCRYPTION},
    {"namedStreams",    FILE_NAMED_STREAMS},
    {"objectIDs",       FILE_SUPPORTS_OBJECT_IDS},
    {"quotas",          FILE_VOLUME_QUOTAS},
    {"readOnly",        FILE_READ_ONLY_VOLUME},
    {"reparsePoints",   FILE_SUPPORTS_REPARSE_POINTS},
    {"sparseFiles",     FILE_SUPPORTS_SPARSE_FILES},
    {"unicode",         FS_UNICODE_STORED_ON_DISK},
#else //_WINDOWS
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
    {"exported",        MNT_EXPORTED},
#elif defined(ST_EXPORTED)
    {"exported",        ST_EXPORTED},
#endif
#ifdef MNT_IGNORE
    {"ignore",          MNT_IGNORE},
#endif
#ifdef MNT_LOCAL
    {"local",           MNT_LOCAL},
#endif
#ifdef ST_LARGEFILES
    {"largeFiles",      ST_LARGEFILES},
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
#elif defined(MS_NOSUID)
    {"noSuid",          MS_NOSUID},
#elif defined(ST_NOSUID)
    {"noSuid",          ST_NOSUID},
#endif
#ifdef MNT_NOSYMFOLLOW
    {"noSymFollow",     MNT_NOSYMFOLLOW},
#endif
#ifdef MNT_QUOTA
    {"quotas",          MNT_QUOTA},
#elif defined(ST_QUOTA)
    {"quotas",          ST_QUOTA},
#endif
#ifdef MNT_RDONLY
    {"readOnly",        MNT_RDONLY},
#elif defined(MS_RDONLY)
    {"readOnly",        MS_RDONLY},
#elif defined(ST_RDONLY)
    {"readOnly",        ST_RDONLY},
#endif
#ifdef MNT_ROOTFS
    {"root",            MNT_ROOTFS},
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
#endif //_WINDOWS
    {NULL}
};


/*++

VolumeObjCmd

    This function provides the "volume" Tcl command.

Arguments:
    dummy  - Not used.

    interp - Current interpreter.

    objc   - Number of arguments.

    objv   - Argument objects.

Return Value:
    A standard Tcl result.

--*/
int
VolumeObjCmd(
    ClientData dummy,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    int index;
    static const char *options[] = {
        "info", "list", NULL
    };
    enum optionIndices {
        OPTION_INFO = 0, OPTION_LIST
    };

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option arg ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch ((enum optionIndices) index) {
        case OPTION_LIST: {
            int i;
            unsigned short listOptions = 0;
            Tcl_Obj *objPtr;
            static const char *switches[] = {
                "-local", "-mounts", "-root", NULL
            };
            enum switchIndices {
                SWITCH_LOCAL = 0, SWITCH_MOUNTS, SWITCH_ROOT
            };

            for (i = 2; i < objc; i++) {
                if (Tcl_GetIndexFromObj(interp, objv[i], switches, "switch", TCL_EXACT, &index) != TCL_OK) {
                    return TCL_ERROR;
                }

                switch ((enum switchIndices) index) {
                    case SWITCH_LOCAL: {
                        listOptions |= VOLLIST_FLAG_LOCAL;
                        break;
                    }
                    case SWITCH_MOUNTS: {
                        listOptions |= VOLLIST_FLAG_MOUNTS;
                        break;
                    }
                    case SWITCH_ROOT: {
                        listOptions |= VOLLIST_FLAG_ROOT;
                        break;
                    }
                }
            }

            // Default to -root if no switches were given.
            if (listOptions == 0) {
                listOptions |= VOLLIST_FLAG_ROOT;
            }

            objPtr = GetVolumeList(interp, listOptions);
            if (objPtr == NULL) {
                return TCL_ERROR;
            }

            Tcl_SetObjResult(interp, objPtr);
            return TCL_OK;
        }
        case OPTION_INFO: {
            Tcl_Obj *fieldObj;
            Tcl_Obj *flagListObj;
            Tcl_Obj *valueObj;
            VolumeInfo volumeInfo;

            if (objc != 4) {
                Tcl_WrongNumArgs(interp, 2, objv, "volume varName");
                return TCL_ERROR;
            }

            memset(&volumeInfo, 0, sizeof(VolumeInfo));
            if (GetVolumeInfo(interp, objv[2], &volumeInfo) != TCL_OK) {
                return TCL_ERROR;
            }

            //
            // Tcl_ObjSetVar2() will not create a copy of the field object,
            // so the caller must free the object once finished with it.
            //
            fieldObj = Tcl_NewObj();
            Tcl_IncrRefCount(fieldObj);

// Easier than repeating this...
#define TCL_STORE_ARRAY(name, value)                                                        \
    valueObj = (value);                                                                     \
    Tcl_SetStringObj(fieldObj, (name), -1);                                                 \
    if (Tcl_ObjSetVar2(interp, objv[3], fieldObj, valueObj, TCL_LEAVE_ERR_MSG) == NULL) {   \
        Tcl_DecrRefCount(fieldObj);                                                         \
        return TCL_ERROR;                                                                   \
    }

            TCL_STORE_ARRAY("length", Tcl_NewLongObj((long)volumeInfo.length));
            TCL_STORE_ARRAY("id",     Tcl_NewWideIntObj((Tcl_WideInt)volumeInfo.id));
            TCL_STORE_ARRAY("name",   Tcl_NewStringObj(volumeInfo.name, -1));
            TCL_STORE_ARRAY("type",   Tcl_NewStringObj(volumeInfo.type, -1));
            TCL_STORE_ARRAY("free",   Tcl_NewWideIntObj((Tcl_WideInt)volumeInfo.free));
            TCL_STORE_ARRAY("used",   Tcl_NewWideIntObj((Tcl_WideInt)(volumeInfo.total-volumeInfo.free)));
            TCL_STORE_ARRAY("total",  Tcl_NewWideIntObj((Tcl_WideInt)volumeInfo.total));

            // Create a list of applicable volume flags.
            flagListObj = Tcl_NewObj();
            for (index = 0; volumeFlags[index].name != NULL; index++) {
                if (volumeInfo.flags & volumeFlags[index].flag) {
                    Tcl_ListObjAppendElement(NULL, flagListObj,
                        Tcl_NewStringObj(volumeFlags[index].name, -1));
                }
            }
            TCL_STORE_ARRAY("flags", flagListObj);

            Tcl_DecrRefCount(fieldObj);
            return TCL_OK;
        }
    }

    // This point is never reached.
    assert(0);
    return TCL_ERROR;
}
