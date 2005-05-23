/*
 * Infernus Library - Tcl extension for the Infernus sitebot.
 * Copyright (c) 2005 Infernus Development Team
 *
 * File Name:
 *   nxUtil.c
 *
 * Author:
 *   neoxed (neoxed@gmail.com) May 22, 2005
 *
 * Abstract:
 *   Miscellanenous Windows specific utilities.
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
