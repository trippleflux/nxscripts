/*
 * AlcoExt - Alcoholicz Tcl extension.
 * Copyright (c) 2005 Alcoholicz Scripting Team
 *
 * File Name:
 *   alcoUtil.c
 *
 * Author:
 *   neoxed (neoxed@gmail.com) May 21, 2005
 *
 * Abstract:
 *   Miscellanenous utility functions.
 */

#include <alcoExt.h>

/*
 * GetHandleTableEntry
 *
 *   Looks up a handle's hash table entry.
 *
 * Arguments:
 *   interp   - Interpreter to use for error reporting.
 *   objPtr   - The string value of this object is used to search through tablePtr.
 *   tablePtr - Address of a hash table structure.
 *   prefix   - Null-terminated string representing the handle identifier's prefix.
 *
 * Returns:
 *   If the handle is valid, the address of the handle's hash table entry is
 *   returned. If the handle is invalid, NULL is returned and an error message
 *   is left in the interpreter's result.
 *
 * Remarks:
 *   None.
 */
Tcl_HashEntry *
GetHandleTableEntry(Tcl_Interp *interp, Tcl_Obj *objPtr, Tcl_HashTable *tablePtr, const char *prefix)
{
    char *handle;
    Tcl_HashEntry *hashEntryPtr = NULL;

    handle = Tcl_GetString(objPtr);
    if (strncmp(handle, prefix, strlen(prefix)) == 0)  {
        hashEntryPtr = Tcl_FindHashEntry(tablePtr, handle);
    }

    if (hashEntryPtr == NULL) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "invalid ", prefix, " handle \"", handle, "\"", NULL);
    }

    return hashEntryPtr;
}

/*
 * PartialSwitchCompare
 *
 *   Performs a partial string comparison for a single switch. This behavior is
 *   consistent with Tcl commands that accept one switch argument, such as
 *   'string match' and 'string map'.
 *
 * Arguments:
 *   objPtr     - The string value of this object is compared against "name".
 *   switchName - Full name of the switch.
 *
 * Returns:
 *   If "name" and the string value of "objPtr" match partially or completely,
 *   the return value is non-zero. If they do not match, the return value is zero.
 *
 * Remarks:
 *   None.
 */
int
PartialSwitchCompare(Tcl_Obj *objPtr, const char *switchName)
{
    int optionLength;
    char *option = Tcl_GetStringFromObj(objPtr, &optionLength);

    /*
     * The user supplied switch must be at least two characters in
     * length, to account for the switch prefix and first letter.
     */
    return (optionLength > 2 && strncmp(switchName, option, optionLength) == 0);
}
