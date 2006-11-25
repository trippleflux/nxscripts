/*
 * nxHelper - Tcl extension for nxTools.
 * Copyright (c) 2004-2006 neoxed
 *
 * File Name:
 *   nxTime.c
 *
 * Author:
 *   neoxed (neoxed@gmail.com) May 22, 2005
 *
 * Abstract:
 *   Implements date and time functions.
 *
 *   Tcl Commands:
 *     ::nx::time dst
 *       - Returns 1 if daylight savings time is currently in affect, and 0 if not.
 *
 *     ::nx::time local (no longer implemented)
 *       - Returns the current local time.
 *
 *     ::nx::time utc (no longer implemented)
 *       - Returns the current time, expressed in UTC.
 *
 *     ::nx::time zone
 *       - Returns the bias for local and UTC time translation, expressed in seconds.
 */

#include <nxHelper.h>

/*
 * TimeObjCmd
 *
 *   This function provides the "::nx::time" Tcl command.
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
TimeObjCmd(
    ClientData dummy,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    int index;
    const static char *options[] = {
        "dst", "local", "utc", "zone", NULL
    };
    enum optionIndices {
        OPTION_DST = 0, OPTION_LOCAL, OPTION_UTC, OPTION_ZONE
    };

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option ?args?");
        return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch ((enum optionIndices) index) {
        case OPTION_DST: {
            TIME_ZONE_INFORMATION timeZoneInfo;

            Tcl_SetIntObj(Tcl_GetObjResult(interp),
                (GetTimeZoneInformation(&timeZoneInfo) == TIME_ZONE_ID_DAYLIGHT));

            return TCL_OK;
        }
        case OPTION_LOCAL: {
            FILETIME localTime;
            FILETIME utcTime;
            GetSystemTimeAsFileTime(&utcTime);
            FileTimeToLocalFileTime(&utcTime, &localTime);

            Tcl_SetLongObj(Tcl_GetObjResult(interp), (long)FileTimeToPosixEpoch(&localTime));
            return TCL_OK;
        }
        case OPTION_UTC: {
            FILETIME utcTime;
            GetSystemTimeAsFileTime(&utcTime);

            Tcl_SetLongObj(Tcl_GetObjResult(interp), (long)FileTimeToPosixEpoch(&utcTime));
            return TCL_OK;
        }
        case OPTION_ZONE: {
            long bias;

            if (GetTimeZoneBias(&bias)) {
                Tcl_SetLongObj(Tcl_GetObjResult(interp), (long)bias);
            } else {
                Tcl_SetLongObj(Tcl_GetObjResult(interp), 0);
            }

            return TCL_OK;
        }
    }

    /* This point is never reached. */
    assert(0);
    return TCL_ERROR;
}
