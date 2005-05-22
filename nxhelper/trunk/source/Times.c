#include <nxHelper.h>


ULONG FileTimeToPosixEpoch(const PFILETIME FileTime)
{
    ULONGLONG Time = ((ULONGLONG)FileTime->dwHighDateTime << 32) + FileTime->dwLowDateTime;
    return (ULONG)((Time - 116444736000000000) / 10000000);
}


VOID PosixEpochToFileTime(ULONG EpochTime, PFILETIME FileTime)
{
    ULONGLONG Time = UInt32x32To64(EpochTime, 10000000) + 116444736000000000;
    FileTime->dwLowDateTime = (DWORD)Time;
    FileTime->dwHighDateTime = (DWORD)(Time >> 32);
}


BOOL GetTimeZoneBias(PLONG Bias)
{
    TIME_ZONE_INFORMATION TimeZoneInfo;

    if (GetTimeZoneInformation(&TimeZoneInfo) != TIME_ZONE_ID_INVALID) {
        *Bias = TimeZoneInfo.Bias * 60;
        return TRUE;
    }

    *Bias = 0;
    return FALSE;
}


INT TclTimeCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[])
{
    INT OptionIndex;
    const static CHAR *Options[] = {"dst", /*"local", "utc",*/ "zone", NULL};
    enum OptionIndexes {OPTION_DST, /*OPTION_LOCAL, OPTION_UTC,*/ OPTION_ZONE};

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option ?args?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], Options, "option", 0, &OptionIndex) != TCL_OK) {
        return TCL_ERROR;
    }
    switch ((enum OptionIndexes) OptionIndex) {
        case OPTION_DST: {
            TIME_ZONE_INFORMATION TimeZoneInfo;

            Tcl_SetIntObj(Tcl_GetObjResult(interp),
                (GetTimeZoneInformation(&TimeZoneInfo) == TIME_ZONE_ID_DAYLIGHT));

            return TCL_OK;
        }
#if 0
        case OPTION_LOCAL: {
            FILETIME LocalTime;
            GetSystemTimeAsFileTime(&LocalTime);
            Tcl_SetLongObj(Tcl_GetObjResult(interp), (LONG)FileTimeToPosixEpoch(&LocalTime));
            return TCL_OK;
        }
        case OPTION_UTC: {
            FILETIME LocalTime;
            FILETIME UtcTime;
            GetSystemTimeAsFileTime(&LocalTime);
            LocalFileTimeToFileTime(&LocalTime, &UtcTime);
            Tcl_SetLongObj(Tcl_GetObjResult(interp), (LONG)FileTimeToPosixEpoch(&UtcTime));
            return TCL_OK;
        }
#endif
        case OPTION_ZONE: {
            LONG Bias;

            if (GetTimeZoneBias(&Bias)) {
                Tcl_SetLongObj(Tcl_GetObjResult(interp), (LONG)Bias);
            } else {
                Tcl_SetLongObj(Tcl_GetObjResult(interp), 0);
            }

            return TCL_OK;
        }
        default: {
            return TCL_ERROR; // Should never be reached.
        }
    }
}
