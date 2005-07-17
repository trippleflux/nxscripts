/*
 * AlcoExt - Alcoholicz Tcl extension.
 * Copyright (c) 2005 Alcoholicz Scripting Team
 *
 * File Name:
 *   alcoWinUtil.c
 *
 * Author:
 *   neoxed (neoxed@gmail.com) April 16, 2005
 *
 * Abstract:
 *   Miscellanenous Windows specific utilities.
 */

#include <alcoExt.h>

#ifndef TCL_TSD_INIT
#define TCL_TSD_INIT(keyPtr) (ThreadSpecificData *)Tcl_GetThreadData((keyPtr), sizeof(ThreadSpecificData))
#endif

typedef struct {
    char systemError[512];
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

    StringCchPrintfA(errorId, ARRAYSIZE(errorId), "%lu", errorCode);

    if (!FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        errorCode,
        0,
        tsdPtr->systemError,
        ARRAYSIZE(tsdPtr->systemError),
        NULL)) {
            StringCchCopyA(tsdPtr->systemError, ARRAYSIZE(tsdPtr->systemError), "unknown error");
    } else {
        /* Remove trailing CR/LF. */
        tsdPtr->systemError[strlen(tsdPtr->systemError)-2] = '\0';
    }

    Tcl_SetErrorCode(interp, "WINDOWS", errorId, tsdPtr->systemError, NULL);
    return tsdPtr->systemError;
}

/*
 * IsFeatureAvailable
 *
 *	 Checks if feature specific functions are available.
 *
 * Arguments:
 *   features - Bit mask of features to check for.
 *
 * Returns:
 *   If the feature is available, the return value is non-zero.
 *   If the feature is not available, the return value is zero.
 *
 * Remarks:
 *   None.
 */
int
IsFeatureAvailable(unsigned long features)
{
    if (features & FEATURE_DISKSPACEEX && winProcs.getDiskFreeSpaceEx == NULL) {
        return 0;
    }

    if (features & FEATURE_MOUNT_POINTS && (
        winProcs.findFirstVolumeMountPoint == NULL ||
        winProcs.findNextVolumeMountPoint  == NULL ||
        winProcs.findVolumeMountPointClose == NULL ||
        winProcs.getVolumeNameForVolumeMountPoint == NULL)) {
        return 0;
    }

    return 1;
}
