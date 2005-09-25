/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005 Alcoholicz Scripting Team

Module Name:
    alcoWinUtil.c

Author:
    neoxed (neoxed@gmail.com) Apr 16, 2005

Abstract:
    Miscellaneous Windows specific utilities.

--*/

#include <alcoExt.h>

#ifndef TCL_TSD_INIT
#define TCL_TSD_INIT(keyPtr) (ThreadSpecificData *)Tcl_GetThreadData((keyPtr), sizeof(ThreadSpecificData))
#endif

typedef struct {
    char message[512];
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;


/*++

TclSetWinError

    Sets the interpreter's errorCode variable.

Arguments:
    interp    - Current interpreter.

    errorCode - Windows error code.

Return Value:
    The message that is associated with the error code.

--*/
char *
TclSetWinError(
    Tcl_Interp *interp,
    unsigned long errorCode
    )
{
    char errorId[12];
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    StringCchPrintfA(errorId, ARRAYSIZE(errorId), "%lu", errorCode);

    if (FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        tsdPtr->message,
        ARRAYSIZE(tsdPtr->message),
        NULL) == 0) {
        StringCchCopyA(tsdPtr->message, ARRAYSIZE(tsdPtr->message), "unknown error");
    } else {
        size_t length;
        StringCchLengthA(tsdPtr->message, ARRAYSIZE(tsdPtr->message), &length);

        // Remove trailing CR/LF.
        if (length >= 2 && tsdPtr->message[length-2] == '\r' && tsdPtr->message[length-1] == '\n') {
            tsdPtr->message[length-2] = '\0';
        }
    }

    Tcl_SetErrorCode(interp, "WINDOWS", errorId, tsdPtr->message, NULL);
    return tsdPtr->message;
}

/*++

IsFeatureAvailable

    Checks if feature specific functions are available.

Arguments:
    features - Bit mask of features to check for.

Return Value:
    If the feature is available, the return value is non-zero.
    If the feature is not available, the return value is zero.

--*/
int
IsFeatureAvailable(
    unsigned long features
    )
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
