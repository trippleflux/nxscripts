/*
 * AlcoExt - Alcoholicz Tcl extension.
 * Copyright (c) 2005 Alcoholicz Scripting Team
 *
 * File Name:
 *   alcoWinIoFtpd.c
 *
 * Author:
 *   neoxed (neoxed@gmail.com) April 16, 2005
 *
 * Abstract:
 *   Implements a Tcl command-based interface for interaction with ioFTPD.
 */

#include <alcoExt.h>

/* Tcl command functions. */
static int IoInfoCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], ExtState *statePtr);
static int IoKickCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], ExtState *statePtr);
static int IoKillCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], ExtState *statePtr);
static int IoResolveCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], ExtState *statePtr);
static int IoWhoCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], ExtState *statePtr);

/* TODO: Add user, gid, and group fields (not available in struct). */
static const char *whoFields[] = {
    "action",
    "cid",
    "host",
    "ident",
    "idletime",
    "ip",
    "logintime",
    "realdatapath",
    "realpath",
    "size",
    "speed",
    "status",
    "uid",
    "vdatapath",
    "vpath",
    NULL
};

enum {
    WHO_ACTION = 0,
    WHO_CID,
    WHO_HOST,
    WHO_IDENT,
    WHO_IDLETIME,
    WHO_IP,
    WHO_LOGINTIME,
    WHO_REALDATAPATH,
    WHO_REALPATH,
    WHO_SIZE,
    WHO_SPEED,
    WHO_STATUS,
    WHO_UID,
    WHO_VDATAPATH,
    WHO_VPATH,
};


/*
 * IoInfoCmd
 *
 *   Retrieves information from an ioFTPD message window.
 *
 * Arguments:
 *   interp   - Current interpreter.
 *   objc     - Number of arguments.
 *   objv     - Argument objects.
 *   statePtr - Pointer to a 'ExtState' structure.
 *
 * Returns:
 *   A standard Tcl result.
 *
 * Remarks:
 *   None.
 */
static int
IoInfoCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], ExtState *statePtr)
{
    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "msgWindow varName");
        return TCL_ERROR;
    }

    /* TODO: IPC stuff. */

    return TCL_OK;
}

/*
 * IoKickCmd
 *
 *   Kicks the specified ioFTPD user.
 *
 * Arguments:
 *   interp   - Current interpreter.
 *   objc     - Number of arguments.
 *   objv     - Argument objects.
 *   statePtr - Pointer to a 'ExtState' structure.
 *
 * Returns:
 *   A standard Tcl result.
 *
 * Remarks:
 *   None.
 */
static int
IoKickCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], ExtState *statePtr)
{
    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "msgWindow user");
        return TCL_ERROR;
    }

    /* TODO: IPC stuff. */

    return TCL_OK;
}

/*
 * IoKillCmd
 *
 *   Kills the specified ioFTPD connection ID.
 *
 * Arguments:
 *   interp   - Current interpreter.
 *   objc     - Number of arguments.
 *   objv     - Argument objects.
 *   statePtr - Pointer to a 'ExtState' structure.
 *
 * Returns:
 *   A standard Tcl result.
 *
 * Remarks:
 *   None.
 */
static int
IoKillCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], ExtState *statePtr)
{
    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "msgWindow uid");
        return TCL_ERROR;
    }

    /* TODO: IPC stuff. */

    return TCL_OK;
}

/*
 * IoResolveCmd
 *
 *   Resolves names and IDs for users and groups.
 *
 *   User  -> UID
 *   UID   -> User
 *   Group -> GID
 *   GID   -> Group
 *
 * Arguments:
 *   interp   - Current interpreter.
 *   objc     - Number of arguments.
 *   objv     - Argument objects.
 *   statePtr - Pointer to a 'ExtState' structure.
 *
 * Returns:
 *   A standard Tcl result.
 *
 * Remarks:
 *   None.
 */
static int
IoResolveCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], ExtState *statePtr)
{
    int index;
    static const char *options[] = {"gid", "group", "uid", "user", NULL};
    enum options {OPTION_GID, OPTION_GROUP, OPTION_UID, OPTION_USER};

    if (objc != 5) {
        Tcl_WrongNumArgs(interp, 2, objv, "option msgWindow value");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[2], options, "option", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch ((enum options) index) {
        case OPTION_GID: {
            break;
        }
        case OPTION_GROUP: {
            break;
        }
        case OPTION_UID: {
            break;
        }
        case OPTION_USER: {
            break;
        }
    }

    /* TODO: IPC stuff. */

    return TCL_OK;
}

/*
 * IoWhoCmd
 *
 *   Retrieves online user information from ioFTPD's shared memory.
 *
 * Arguments:
 *   interp   - Current interpreter.
 *   objc     - Number of arguments.
 *   objv     - Argument objects.
 *   statePtr - Pointer to a 'ExtState' structure.
 *
 * Returns:
 *   A standard Tcl result.
 *
 * Remarks:
 *   None.
 */
static int
IoWhoCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], ExtState *statePtr)
{
    int elementCount;
    int fieldIndex;
    int i;
    int result;
    unsigned char *fields;
    Tcl_Obj **elementPtrs;

    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "msgWindow fields");
        return TCL_ERROR;
    }

    if (Tcl_ListObjGetElements(interp, objv[3], &elementCount, &elementPtrs) != TCL_OK) {
        return TCL_ERROR;
    }

    /* Create an array of indices from 'whoFields'. */
    result = TCL_ERROR;
    fields = (unsigned char *) ckalloc(elementCount);

    for (i = 0; i < elementCount; i++) {
        if (Tcl_GetIndexFromObj(interp, elementPtrs[i], whoFields, "field", 0,
            &fieldIndex) != TCL_OK) {
            goto end;
        }

        fields[i] = (unsigned char) fieldIndex;
    }

    /* TODO: IPC stuff. */
    result = TCL_OK;

    end:
    ckfree((char *) fields);
    return result;
}

/*
 *  IoFtpdObjCmd
 *
 *	This function provides the "ioftpd" Tcl command.
 *
 * Arguments:
 *   clientData - Pointer to a 'ExtState' structure.
 *   interp     - Current interpreter.
 *   objc       - Number of arguments.
 *   objv       - Argument objects.
 *
 * Returns:
 *   A standard Tcl result.
 *
 * Remarks:
 *   None.
 */
int
IoFtpdObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    ExtState *statePtr = (ExtState *) clientData;
    int index;
    static const char *options[] = {
        "info", "kick", "kill", "resolve", "who", NULL
    };
    enum options {
        OPTION_INFO, OPTION_KICK, OPTION_KILL, OPTION_RESOLVE, OPTION_WHO
    };

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option arg ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch ((enum options) index) {
        case OPTION_INFO:    return IoInfoCmd(interp, objc, objv, statePtr);
        case OPTION_KICK:    return IoKickCmd(interp, objc, objv, statePtr);
        case OPTION_KILL:    return IoKillCmd(interp, objc, objv, statePtr);
        case OPTION_RESOLVE: return IoResolveCmd(interp, objc, objv, statePtr);
        case OPTION_WHO:     return IoWhoCmd(interp, objc, objv, statePtr);
    }

    /* This point should never be reached. */
    Tcl_Panic("unexpected fallthrough");
    return TCL_ERROR;
}
