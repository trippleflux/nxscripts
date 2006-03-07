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
 *     ::nx::var exists <key>
 *     ::nx::var get    <key>
 *     ::nx::var list
 *     ::nx::var set    <key> <value>
 *     ::nx::var unset  [-nocomplain] <key>
 */

#include <nxHelper.h>

typedef struct {
    int length;
    unsigned char *data;
} VarValue;

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


/*
 * VarClearTable
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
VarClearTable(
    void
    )
{
    Tcl_HashSearch search;
    Tcl_HashEntry *hashEntry;

    for (hashEntry = Tcl_FirstHashEntry(varTable, &search);
            hashEntry != NULL;
            hashEntry = Tcl_NextHashEntry(&search)) {

        ckfree((char *)Tcl_GetHashValue(hashEntry));
        Tcl_DeleteHashEntry(hashEntry);
    }
}


/* ::nx::var exists <key> */
static int
VarExists(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    char *name;
    Tcl_HashEntry *hashEntry;
    Tcl_Obj *resultObj;

    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "key");
        return TCL_ERROR;
    }
    name = Tcl_GetString(objv[2]);
    resultObj = Tcl_GetObjResult(interp);

    /* Look-up the variable's hash table entry. */
    Tcl_MutexLock(&varMutex);
    hashEntry = Tcl_FindHashEntry(varTable, name);
    Tcl_MutexUnlock(&varMutex);

    Tcl_SetBooleanObj(resultObj, (hashEntry != NULL) ? 1 : 0);
    return TCL_OK;
}

/* ::nx::var get <key> */
static int
VarGet(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    char *name;
    Tcl_HashEntry *hashEntry;
    Tcl_Obj *resultObj;
    VarValue *value;

    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "key");
        return TCL_ERROR;
    }
    name = Tcl_GetString(objv[2]);
    resultObj = Tcl_GetObjResult(interp);

    /* Look-up the variable's hash table entry. */
    Tcl_MutexLock(&varMutex);
    hashEntry = Tcl_FindHashEntry(varTable, name);

    if (hashEntry != NULL) {
        /* Duplicate variable value. */
        value = (VarValue *)Tcl_GetHashValue(hashEntry);
        Tcl_SetByteArrayObj(resultObj, value->data, value->length);
    }
    Tcl_MutexUnlock(&varMutex);

    if (hashEntry == NULL) {
        Tcl_AppendResult(interp, "invalid key \"", name, "\"", NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/* ::nx::var list */
static int
VarList(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    char *name;
    Tcl_HashEntry *hashEntry;
    Tcl_HashSearch hashSearch;
    Tcl_Obj *resultObj;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
    }
    resultObj = Tcl_GetObjResult(interp);

    /* Create a list of all variables. */
    Tcl_MutexLock(&varMutex);
    for (hashEntry = Tcl_FirstHashEntry(varTable, &hashSearch);
            hashEntry != NULL;
            hashEntry = Tcl_NextHashEntry(&hashSearch)) {

        name = Tcl_GetHashKey(varTable, hashEntry);
        Tcl_ListObjAppendElement(NULL, resultObj, Tcl_NewStringObj(name, -1));
    }
    Tcl_MutexUnlock(&varMutex);

    return TCL_OK;
}

/* ::nx::var set <key> <value> */
static int
VarSet(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    char *name;
    int dataLength;
    int newEntry;
    Tcl_HashEntry *hashEntry;
    unsigned char *data;
    VarValue *value;

    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "key value");
        return TCL_ERROR;
    }
    name = Tcl_GetString(objv[2]);
    data = Tcl_GetByteArrayFromObj(objv[3], &dataLength);

    /* Allocate and populate the VarValue structure. */
    value = (VarValue *)ckalloc(sizeof(VarValue) + dataLength);
    value->data = (unsigned char *)&value[1];
    value->length = dataLength;
    memcpy(value->data, data, dataLength);

    /* Create a hash table entry and update its value. */
    Tcl_MutexLock(&varMutex);
    hashEntry = Tcl_CreateHashEntry(varTable, name, &newEntry);
    if (newEntry == 0) {
        /* Free the current value. */
        ckfree((char *)Tcl_GetHashValue(hashEntry));
    }
    Tcl_SetHashValue(hashEntry, (ClientData)value);
    Tcl_MutexUnlock(&varMutex);

    return TCL_OK;
}

/* ::nx::var unset [-nocomplain] <key> */
static int
VarUnset(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    char *name;
    int complain = 1;
    Tcl_HashEntry *hashEntry;

    switch (objc) {
        case 3: {
            break;
        }
        case 4: {
            if (PartialSwitchCompare(objv[2], "-nocomplain")) {
                complain = 0;
                break;
            }
        }
        default: {
            Tcl_WrongNumArgs(interp, 2, objv, "?-nocomplain? key");
            return TCL_ERROR;
        }
    }
    name = Tcl_GetString(objv[2]);

    /* Remove the hash table entry. */
    Tcl_MutexLock(&varMutex);
    hashEntry = Tcl_FindHashEntry(varTable, name);

    if (hashEntry != NULL) {
        ckfree((char *)Tcl_GetHashValue(hashEntry));
        Tcl_DeleteHashEntry(hashEntry);
    }
    Tcl_MutexUnlock(&varMutex);

    if (complain && hashEntry == NULL) {
        Tcl_AppendResult(interp, "invalid key \"", name, "\"", NULL);
        return TCL_ERROR;
    }
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
        Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
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
