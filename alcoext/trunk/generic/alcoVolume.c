/*
 * AlcoExt - Alcoholicz Tcl extension.
 * Copyright (c) 2005 Alcoholicz Scripting Team
 *
 * File Name:
 *   alcoVolume.c
 *
 * Author:
 *   neoxed (neoxed@gmail.com) June 2, 2005
 *
 * Abstract:
 *   Implements a Tcl interface to retrieve volume information.
 *
 *   Tcl Commands:
 *     volume list [-local] [-mounts] [-root]
 *       - Retrieves a list of volumes and mount points.
 *
 *     volume info <volume> <varName>
 *      - Retrieves information for the given volume, and uses the
 *        variable given by varName to store the returned information.
 *      - Array Contents:
 *        flags  - A list of lags associated with the file system.
 *        free   - Remaining space, expressed in bytes.
 *        id     - Volume identification number.
 *        length - Maximum length of a file name.
 *        name   - Name of the volume.
 *        total  - Total space, expressed in bytes.
 *        type   - Volume type and file system name.
 */

#include <alcoExt.h>

/*
 * VolumeObjCmd
 *
 *	 This function provides the "volume" Tcl command.
 *
 * Arguments:
 *   dummy  - Not used.
 *   interp - Current interpreter.
 *   objc   - Number of arguments.
 *   objv   - Argument objects.
 *
 * Returns:
 *   A standard Tcl result.
 *
 * Remarks:
 *   None.
 */
int
VolumeObjCmd(ClientData dummy, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    int index;
    static const char *options[] = {"info", "list", NULL};
    enum options {OPTION_INFO, OPTION_LIST};

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option arg ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch ((enum options) index) {
        case OPTION_LIST: {
            int i;
            unsigned short listOptions = 0;
            Tcl_Obj *objPtr;
            static const char *switches[] = {"-local", "-mounts", "-root", NULL};
            enum switches {SWITCH_LOCAL, SWITCH_MOUNTS, SWITCH_ROOT};

            for (i = 2; i < objc; i++) {
                if (Tcl_GetIndexFromObj(interp, objv[i], switches, "switch", TCL_EXACT, &index) != TCL_OK) {
                    return TCL_ERROR;
                }

                switch ((enum switches) index) {
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

            /* Default to -root if no switches were given. */
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
            if (GetVolumeInfo(interp, Tcl_GetString(objv[2]), &volumeInfo) != TCL_OK) {
                return TCL_ERROR;
            }

            /*
             * Tcl_ObjSetVar2() won't create a copy of the field object,
             * so the caller must free the object once finished with it.
             */
            fieldObj = Tcl_NewObj();
            Tcl_IncrRefCount(fieldObj);

/* Easier than repeating this... */
#define TCL_STORE_ARRAY(name, value)                                                        \
    valueObj = (value);                                                                     \
    Tcl_SetStringObj(fieldObj, (name), -1);                                                 \
    if (Tcl_ObjSetVar2(interp, objv[3], fieldObj, valueObj, TCL_LEAVE_ERR_MSG) == NULL) {   \
        Tcl_DecrRefCount(fieldObj);                                                         \
        Tcl_DecrRefCount(valueObj);                                                         \
        return TCL_ERROR;                                                                   \
    }

            TCL_STORE_ARRAY("length", Tcl_NewLongObj((long)volumeInfo.length));
            TCL_STORE_ARRAY("id",     Tcl_NewWideIntObj((Tcl_WideInt)volumeInfo.id));
            TCL_STORE_ARRAY("name",   Tcl_NewStringObj(volumeInfo.name, -1));
            TCL_STORE_ARRAY("type",   Tcl_NewStringObj(volumeInfo.type, -1));
            TCL_STORE_ARRAY("free",   Tcl_NewWideIntObj((Tcl_WideInt)volumeInfo.free));
            TCL_STORE_ARRAY("total",  Tcl_NewWideIntObj((Tcl_WideInt)volumeInfo.total));

            /* Create a list of applicable volume flags. */
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

    /* This point should never be reached. */
    Tcl_Panic("unexpected fallthrough");
    return TCL_ERROR;
}
