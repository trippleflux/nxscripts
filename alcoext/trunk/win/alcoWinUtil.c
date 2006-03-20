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

TclGetOctalFromObj

    Retrieves an octal value from the given object.

Arguments:
    interp   - Interpreter to use for error reporting.

    objPtr   - Object to retrieve the octal value from.

    octalPtr - Address to store the octal value.

Return Value:
    A standard Tcl result.

--*/
static int
TclGetOctalFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,
    unsigned long *octalPtr
    )
{
    char *input;

    assert(interp   != NULL);
    assert(objPtr   != NULL);
    assert(octalPtr != NULL);

    input = Tcl_GetString(objPtr);
    *octalPtr = strtoul(input, NULL, 8);

    if (errno == ERANGE) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "expected octal but got \"", input, "\"", NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*++

TclNewOctalObj

    Creates a new object from an octal value.

Arguments:
    octal   - The octal value to use.

Return Value:
    A pointer to a newly created object.

--*/
static Tcl_Obj *
TclNewOctalObj(
    unsigned long octal
    )
{
    char value[12];
    StringCchPrintfA(value, ARRAYSIZE(value), "%lo", octal);
    return Tcl_NewStringObj(value, -1);
}

/*++

TclSetOctalObj

    Sets the object to an octal value.

Arguments:
    objPtr  - Object to replace.

    octal   - The octal value to use.

Return Value:
    None.

--*/
static void
TclSetOctalObj(
    Tcl_Obj *objPtr,
    unsigned long octal
    )
{
    char value[12];
    assert(objPtr != NULL);

    StringCchPrintfA(value, ARRAYSIZE(value), "%lo", octal);
    Tcl_SetStringObj(objPtr, value, -1);
}
