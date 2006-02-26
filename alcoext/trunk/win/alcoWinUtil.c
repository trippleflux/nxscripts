/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Windows Utilities

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
    ThreadSpecificData *tsdPtr;

    assert(interp != NULL);

    tsdPtr = TCL_TSD_INIT(&dataKey);
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

FileTimeToEpoch

    Convert a FILETIME structure to a POSIX epoch time.

Arguments:
    fileTime    - Pointer to a FILETIME structure.

Return Value:
    The POSIX epoch time.

--*/
long
FileTimeToEpoch(
    const FILETIME *fileTime
    )
{
    ULONGLONG epochTime;
    assert(fileTime != NULL);

    epochTime = ((ULONGLONG)fileTime->dwHighDateTime << 32) + fileTime->dwLowDateTime;
    return (long)((epochTime - 116444736000000000) / 10000000);
}


/*++

EpochToFileTime

    Convert a POSIX epoch time to a FILETIME structure.

Arguments:
    epochTime   - POSIX epoch time.

    fileTime    - Pointer to a FILETIME structure.

Return Value:
    None.

--*/
void
EpochToFileTime(
    long epochTime,
    FILETIME *fileTime
    )
{
    ULONGLONG timeNs;
    assert(fileTime != NULL);

    timeNs = UInt32x32To64(epochTime, 10000000) + 116444736000000000;
    fileTime->dwLowDateTime = (DWORD)timeNs;
    fileTime->dwHighDateTime = (DWORD)(timeNs >> 32);
}
