/*
 * nxHelper - Tcl extension for nxTools.
 * Copyright (c) 2004-2006 neoxed
 *
 * File Name:
 *   nxVar.c
 *
 * Author:
 *   neoxed (neoxed@gmail.com) Mar 7, 2006
 *
 * Abstract:
 *   Implements global-interp variable functions.
 *
 *   Tcl Commands:
 *     ::nx::var exists <variable>
 *     ::nx::var get    <variable>
 *     ::nx::var set    <variable> <value>
 *     ::nx::var unset  <variable>
 */

#include <nxHelper.h>

/*
 * VarFree
 *
 *	 Frees all variable entries in the given hash table.
 *
 * Arguments:
 *   tablePtr - Variable hash table.
 *
 * Returns:
 *   None.
 */
void
VarFree(
    Tcl_HashTable *tablePtr
    )
{
    Tcl_HashSearch search;
    Tcl_HashEntry *entryPtr;

    for (entryPtr = Tcl_FirstHashEntry(tablePtr, &search);
            entryPtr != NULL;
            entryPtr = Tcl_NextHashEntry(&search)) {

        ckfree((char *)Tcl_GetHashValue(entryPtr));
        Tcl_DeleteHashEntry(entryPtr);
    }
}

/*
 * VarObjCmd
 *
 *	 This function provides the "::nx::var" Tcl command.
 *
 * Arguments:
 *   dummy  - Not used.
 *   interp - Current interpreter.
 *   objc   - Number of arguments.
 *   objv   - Argument objects.
 *
 * Returns:
 *   A standard Tcl result.
 */
int
VarObjCmd(
    ClientData dummy,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    int index;
    static const char *options[] = {
        "exists", "get", "set", "unset", NULL
    };
    enum optionIndices {
        OPTION_EXISTS = 0, OPTION_GET, OPTION_SET, OPTION_UNSET
    };

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option arg ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch ((enum optionIndices) index) {
        case OPTION_EXISTS:
        case OPTION_GET:
        case OPTION_SET:
        case OPTION_UNSET:
            // TODO
            break;
    }

    /* This point is never reached. */
    assert(0);
    return TCL_ERROR;
}
