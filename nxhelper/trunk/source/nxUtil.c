/*
 * nxHelper - Tcl extension for nxTools.
 * Copyright (c) 2004-2008 neoxed
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

typedef struct {
    char message[512];
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;


/*
 * TclSetWinError
 *
 *   Sets the interpreter's errorCode variable.
 *
 * Arguments:
 *   interp    - Current interpreter.
 *   errorCode - Windows error code.
 *
 * Returns:
 *   The message that is associated with the error code.
 */
char *
TclSetWinError(
    Tcl_Interp *interp,
    DWORD errorCode
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

        /* Remove trailing CR/LF. */
        if (length >= 2 && tsdPtr->message[length-2] == '\r' && tsdPtr->message[length-1] == '\n') {
            tsdPtr->message[length-2] = '\0';
        }
    }

    Tcl_SetErrorCode(interp, "WINDOWS", errorId, tsdPtr->message, NULL);
    return tsdPtr->message;
}

/*
 * TclSwitchCompare
 *
 *   Performs a partial string comparison for a single switch. This behavior
 *   is consistent with Tcl commands that accept one switch argument, such
 *   as 'string match' and 'string map'.
 *
 * Arguments:
 *   objPtr     - The string value of this object is compared against "name".
 *   switchName - Full name of the switch.
 *
 * Returns:
 *   If "name" and the string value of "objPtr" match partially or completely,
 *   the return value is non-zero. If they do not match, the return value is zero.
 */
int
TclSwitchCompare(
    Tcl_Obj *objPtr,
    const char *switchName
    )
{
    int optionLength;
    char *option = Tcl_GetStringFromObj(objPtr, &optionLength);

    /*
     * The user supplied switch must be at least two characters in
     * length, to account for the switch prefix and first letter.
     */
    return (optionLength > 2 && strncmp(switchName, option, optionLength) == 0);
}

/*
 * TclGetPathFromObj
 *
 *   Translates a file path from a given object.
 *
 * Arguments:
 *   interp  - Interpreter to use for error reporting.
 *
 *   objPtr  - Object containing the file path to be translated.
 *
 *   buffer  - Dynamic string buffer to receive the translated path.
 *
 * Return Value:
 *   A standard Tcl result.
 */
#if 0
int
TclGetPathFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,
    Tcl_DString *buffer
    )
{
    char    *path;
    int     pathLenth;
    Tcl_Obj *translatedObj;

    assert(interp  != NULL);
    assert(objPtr != NULL);
    assert(buffer  != NULL);

    translatedObj = Tcl_FSGetTranslatedPath(interp, objPtr);
    if (translatedObj == NULL) {
        return TCL_ERROR;
    }

    /* Create a dynamic string from the translated path. */
    Tcl_DStringInit(buffer);
    path = Tcl_GetStringFromObj(translatedObj, &pathLenth);
    Tcl_DStringAppend(buffer, path, pathLenth);
    Tcl_DecrRefCount(translatedObj);

#ifdef _WINDOWS
    {
        char *p = Tcl_DStringValue(buffer);

        /* Convert forward slashes to backslashes for Windows paths. */
        while (*p) {
            if (*p == '/') {
                *p = '\\';
            }
            p++;
        }
    }
#endif /* _WINDOWS */

    return TCL_OK;
}
#endif


BOOL
GetTimeZoneBias(
    long *bias
    )
{
    TIME_ZONE_INFORMATION timeZoneInfo;

    if (GetTimeZoneInformation(&timeZoneInfo) != TIME_ZONE_ID_INVALID) {
        *bias = timeZoneInfo.Bias * 60;
        return TRUE;
    }

    *bias = 0;
    return FALSE;
}

unsigned long
FileTimeToPosixEpoch(
    const FILETIME *FileTime
    )
{
    ULONGLONG epochTime = ((ULONGLONG)FileTime->dwHighDateTime << 32) + FileTime->dwLowDateTime;
    return (unsigned long)((epochTime - 116444736000000000) / 10000000);
}

void
PosixEpochToFileTime(
    unsigned long epochTime,
    FILETIME *fileTime
    )
{
    ULONGLONG timeNs = UInt32x32To64(epochTime, 10000000) + 116444736000000000;
    fileTime->dwLowDateTime = (DWORD)timeNs;
    fileTime->dwHighDateTime = (DWORD)(timeNs >> 32);
}


/*
 * SleepObjCmd
 *
 *   This function provides the "::nx::sleep" Tcl command.
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
SleepObjCmd(
    ClientData dummy,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    long ms;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "ms");
        return TCL_ERROR;
    }

    if (Tcl_GetLongFromObj(interp, objv[1], &ms) != TCL_OK) {
        return TCL_ERROR;
    }

    Sleep((DWORD) ms);

    return TCL_OK;
}
