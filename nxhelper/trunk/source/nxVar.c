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
 *     ::nx::var list
 *     ::nx::var set    <variable> <value>
 *     ::nx::var unset  <variable>
 */

#include <nxHelper.h>

/* Tcl command functions. */

static int
VarExists(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    );

static int
VarGet(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    );

static int
VarList(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    );

static int
VarSet(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    );

static int
VarUnset(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    );

/* Utility functions. */

static Tcl_HashEntry *
VarTableGet(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr
    );


/*
 * VarTableClear
 *
 *	 Clear all variable hash table entries.
 *
 * Arguments:
 *	 None.
 *
 * Returns:
 *	 None.
 */
void
VarTableClear(
    void
    )
{
    Tcl_HashSearch search;
    Tcl_HashEntry *entryPtr;

    for (entryPtr = Tcl_FirstHashEntry(varTable, &search);
            entryPtr != NULL;
            entryPtr = Tcl_NextHashEntry(&search)) {

        ckfree((char *)Tcl_GetHashValue(entryPtr));
        Tcl_DeleteHashEntry(entryPtr);
    }
}

/*
 * VarTableGet
 *
 *	 Looks up a variable's hash table entry.
 *
 * Arguments:
 *	 interp - Interpreter to use for error reporting.
 *	 objPtr - The string value of this object is used to search the hash table.
 *
 * Returns:
 *	 If the variable exists, the address of the variable's hash table entry is
 *	 returned. If the variable does not exist, NULL is returned and an error
 *	 message is left in the interpreter's result.
 */
static Tcl_HashEntry *
VarTableGet(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr
    )
{
    char *variable;
    Tcl_HashEntry *hashEntryPtr;

    variable = Tcl_GetString(objPtr);
    hashEntryPtr = Tcl_FindHashEntry(varTable, variable);

    if (hashEntryPtr == NULL) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "invalid variable \"", variable, "\"", NULL);
    }

    return hashEntryPtr;
}


static int
VarExists(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    return TCL_OK;
}

static int
VarGet(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    return TCL_OK;
}

static int
VarList(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    return TCL_OK;
}

static int
VarSet(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    return TCL_OK;
}

static int
VarUnset(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    return TCL_OK;
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
        "exists", "get", "list", "set", "unset", NULL
    };
    enum optionIndices {
        OPTION_EXISTS = 0, OPTION_GET, OPTION_LIST, OPTION_SET, OPTION_UNSET
    };

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option arg ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch ((enum optionIndices) index) {
        case OPTION_EXISTS: return VarExists(interp, objc, objv);
        case OPTION_GET:    return VarGet(interp, objc, objv);
        case OPTION_LIST:   return VarList(interp, objc, objv);
        case OPTION_SET:    return VarSet(interp, objc, objv);
        case OPTION_UNSET:  return VarUnset(interp, objc, objv);
    }

    /* This point is never reached. */
    assert(0);
    return TCL_ERROR;
}
