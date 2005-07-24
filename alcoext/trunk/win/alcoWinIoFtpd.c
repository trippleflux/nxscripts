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
    /* TODO */
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
    /* TODO */
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
    /* TODO */
    return TCL_OK;
}

/*
 * IoResolveCmd
 *
 *   Resolves names and IDs for users and groups.
 *
 *   user name  -> user ID
 *   user ID    -> user name
 *   group name -> group ID
 *   group ID   -> group name
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
    /* TODO */
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
    /* TODO */
    return TCL_OK;
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
