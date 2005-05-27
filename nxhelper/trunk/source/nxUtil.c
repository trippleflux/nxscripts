/*
 * nxHelper - Tcl extension for nxTools.
 * Copyright (c) 2005 neoxed
 *
 * File Name:
 *   nxUtil.c
 *
 * Author:
 *   neoxed (neoxed@gmail.com) May 22, 2005
 *
 * Abstract:
 *   Miscellanenous utilities.
 */

#include <nxHelper.h>

#ifndef TCL_TSD_INIT
#define TCL_TSD_INIT(keyPtr) (ThreadSpecificData *)Tcl_GetThreadData((keyPtr), sizeof(ThreadSpecificData))
#endif

#define ERROR_BUFFER_SIZE 512
typedef struct {
    char systemError[ERROR_BUFFER_SIZE];
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;


/*
 * TclSetWinError
 *
 *	 Sets the interpreter's errorCode variable.
 *
 * Arguments:
 *   interp    - Current interpreter.
 *   errorCode - Windows error code.
 *
 * Returns:
 *   The message that is associated with the error code.
 *
 * Remarks:
 *   None.
 */

char *
TclSetWinError(Tcl_Interp *interp, unsigned long errorCode)
{
    char errorId[12];
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    StringCchPrintfA(errorId, 12, "%lu", errorCode);

    if (!FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK,
        NULL,
        errorCode,
        0,
        tsdPtr->systemError,
        ERROR_BUFFER_SIZE,
        NULL)) {
            StringCchCopyA(tsdPtr->systemError, ERROR_BUFFER_SIZE, "unknown error");
    }

    Tcl_SetErrorCode(interp, "WINDOWS", errorId, tsdPtr->systemError, NULL);
    return tsdPtr->systemError;
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
     * The user supplied switch must be at least one character
     * in length to account for the switch prefix (hyphen).
     */
    return (optionLength > 1 && strncmp(switchName, option, optionLength) == 0);
}
