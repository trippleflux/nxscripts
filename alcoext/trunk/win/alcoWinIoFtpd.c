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

/*
 *  IoFtpdObjCmd
 *
 *	This function provides the "ioftpd" Tcl command.
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
IoFtpdObjCmd(ClientData dummy, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    /* TODO: Finish command when ioFTPD Beta 6 arrives. */

    Tcl_SetResult(interp, "not implemented", TCL_STATIC);
    return TCL_ERROR;
}
