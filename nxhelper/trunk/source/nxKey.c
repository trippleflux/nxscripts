/*
 * nxHelper - Tcl extension for nxTools.
 * Copyright (c) 2004-2008 neoxed
 *
 * File Name:
 *   nxKey.c
 *
 * Author:
 *   neoxed (neoxed@gmail.com) Mar 7, 2006
 *
 * Abstract:
 *   Implements key functions, to share data between interpreters.
 *
 *   Tcl Commands:
 *     ::nx::key exists <name>
 *     ::nx::key get    <name>
 *     ::nx::key list
 *     ::nx::key set    <name> <value>
 *     ::nx::key unset  [-nocomplain] <name>
 */

#include <nxHelper.h>

typedef struct {
    int length;             /* Length of data, in bytes. */
    unsigned char *data;    /* Value, do NOT free this member. */
} KeyValue;


/* ::nx::key exists <name> */
static int
KeyExists(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    char *name;
    Tcl_HashEntry *hashEntry;
    Tcl_Obj *resultObj;

    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "name");
        return TCL_ERROR;
    }
    name = Tcl_GetString(objv[2]);
    resultObj = Tcl_GetObjResult(interp);

    /* Look-up the key's hash table entry. */
    Tcl_MutexLock(&keyMutex);
    hashEntry = Tcl_FindHashEntry(keyTable, name);
    Tcl_MutexUnlock(&keyMutex);

    Tcl_SetBooleanObj(resultObj, (hashEntry != NULL) ? 1 : 0);
    return TCL_OK;
}

/* ::nx::key get <name> */
static int
KeyGet(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    char *name;
    KeyValue *value;
    Tcl_HashEntry *hashEntry;
    Tcl_Obj *resultObj;

    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "name");
        return TCL_ERROR;
    }
    name = Tcl_GetString(objv[2]);
    resultObj = Tcl_GetObjResult(interp);

    /* Look-up the key's hash table entry. */
    Tcl_MutexLock(&keyMutex);
    hashEntry = Tcl_FindHashEntry(keyTable, name);

    if (hashEntry != NULL) {
        /* Copy value to the result object. */
        value = (KeyValue *)Tcl_GetHashValue(hashEntry);
        Tcl_SetByteArrayObj(resultObj, value->data, value->length);
    }
    Tcl_MutexUnlock(&keyMutex);

    if (hashEntry == NULL) {
        Tcl_AppendResult(interp, "invalid key name \"", name, "\"", NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/* ::nx::key list */
static int
KeyList(
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

    /* Create a list of all key names. */
    Tcl_MutexLock(&keyMutex);
    for (hashEntry = Tcl_FirstHashEntry(keyTable, &hashSearch);
            hashEntry != NULL;
            hashEntry = Tcl_NextHashEntry(&hashSearch)) {

        name = Tcl_GetHashKey(keyTable, hashEntry);
        Tcl_ListObjAppendElement(NULL, resultObj, Tcl_NewStringObj(name, -1));
    }
    Tcl_MutexUnlock(&keyMutex);

    return TCL_OK;
}

/* ::nx::key set <name> <value> */
static int
KeySet(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    char *name;
    int dataLength;
    int newEntry;
    KeyValue *value;
    Tcl_HashEntry *hashEntry;
    unsigned char *data;

    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "name value");
        return TCL_ERROR;
    }
    name = Tcl_GetString(objv[2]);
    data = Tcl_GetByteArrayFromObj(objv[3], &dataLength);

    /* Allocate and fill the value structure. */
    value = (KeyValue *)ckalloc(sizeof(KeyValue) + dataLength);
    value->data = (unsigned char *)&value[1];
    value->length = dataLength;
    memcpy(value->data, data, dataLength);

    /* Create a hash table entry and update its value. */
    Tcl_MutexLock(&keyMutex);
    hashEntry = Tcl_CreateHashEntry(keyTable, name, &newEntry);
    if (newEntry == 0) {
        /* Free the current value. */
        ckfree((char *)Tcl_GetHashValue(hashEntry));
    }
    Tcl_SetHashValue(hashEntry, (ClientData)value);
    Tcl_MutexUnlock(&keyMutex);

    return TCL_OK;
}

/* ::nx::key unset [-nocomplain] <name> */
static int
KeyUnset(
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
            if (TclSwitchCompare(objv[2], "-nocomplain")) {
                complain = 0;
                break;
            }
        }
        default: {
            Tcl_WrongNumArgs(interp, 2, objv, "?-nocomplain? name");
            return TCL_ERROR;
        }
    }
    name = Tcl_GetString(objv[objc-1]);

    /* Remove the hash table entry. */
    Tcl_MutexLock(&keyMutex);
    hashEntry = Tcl_FindHashEntry(keyTable, name);

    if (hashEntry != NULL) {
        ckfree((char *)Tcl_GetHashValue(hashEntry));
        Tcl_DeleteHashEntry(hashEntry);
    }
    Tcl_MutexUnlock(&keyMutex);

    if (complain && hashEntry == NULL) {
        Tcl_AppendResult(interp, "invalid key name \"", name, "\"", NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}


/*
 * KeyClearTable
 *
 *   Clear all key hash table entries.
 *
 * Arguments:
 *   None.
 *
 * Returns:
 *   None.
 */
void
KeyClearTable(
    void
    )
{
    Tcl_HashSearch search;
    Tcl_HashEntry *hashEntry;

    for (hashEntry = Tcl_FirstHashEntry(keyTable, &search);
            hashEntry != NULL;
            hashEntry = Tcl_NextHashEntry(&search)) {

        ckfree((char *)Tcl_GetHashValue(hashEntry));
        Tcl_DeleteHashEntry(hashEntry);
    }
}

/*
 * KeyObjCmd
 *
 *   This function provides the "::nx::key" Tcl command.
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
KeyObjCmd(
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
        case OPTION_EXISTS: return KeyExists(interp, objc, objv);
        case OPTION_GET:    return KeyGet(interp, objc, objv);
        case OPTION_LIST:   return KeyList(interp, objc, objv);
        case OPTION_SET:    return KeySet(interp, objc, objv);
        case OPTION_UNSET:  return KeyUnset(interp, objc, objv);
    }

    /* This point is never reached. */
    assert(0);
    return TCL_ERROR;
}
