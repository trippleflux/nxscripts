#include <nxHelper.h>

ULONG FileTimeToPosixEpoch(const LPFILETIME pFileTime)
{
    ULONGLONG ullTime = ((ULONGLONG)pFileTime->dwHighDateTime << 32) + pFileTime->dwLowDateTime;
    return (ULONG)((ullTime - 116444736000000000) / 10000000);
}

VOID PosixEpochToFileTime(ULONG ulEpochTime, LPFILETIME pFileTime)
{
    ULONGLONG ullTime = UInt32x32To64(ulEpochTime, 10000000) + 116444736000000000;
    pFileTime->dwLowDateTime = (DWORD)ullTime;
    pFileTime->dwHighDateTime = (DWORD)(ullTime >> 32);
}

BOOL GetTimeZoneBias(LPLONG plBias)
{
    TIME_ZONE_INFORMATION tzi;

    if (GetTimeZoneInformation(&tzi) != TIME_ZONE_ID_INVALID) {
        *plBias = tzi.Bias * 60;
        return TRUE;
    }

    *plBias = 0;
    return FALSE;
}

INT TclTimeCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[])
{
    INT nIndex;
    const static CHAR *szOptions[] = {"dst", /*"local", "utc",*/ "zone", NULL};
    enum eOptions {OPTION_DST, /*OPTION_LOCAL, OPTION_UTC,*/ OPTION_ZONE};

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option ?args?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], szOptions, "option", 0, &nIndex) != TCL_OK) {
        return TCL_ERROR;
    }
    switch ((enum eOptions) nIndex) {
        case OPTION_DST: {
            TIME_ZONE_INFORMATION tzi;

            Tcl_SetIntObj(Tcl_GetObjResult(interp),
                (GetTimeZoneInformation(&tzi) == TIME_ZONE_ID_DAYLIGHT));

            return TCL_OK;
        }
#if 0
        case OPTION_LOCAL: {
            FILETIME ftLocal;
            GetSystemTimeAsFileTime(&ftLocal);
            Tcl_SetLongObj(Tcl_GetObjResult(interp), (LONG)FileTimeToPosixEpoch(&ftLocal));
            return TCL_OK;
        }
        case OPTION_UTC: {
            FILETIME ftLocal, ftUTC;
            GetSystemTimeAsFileTime(&ftLocal);
            LocalFileTimeToFileTime(&ftLocal, &ftUTC);
            Tcl_SetLongObj(Tcl_GetObjResult(interp), (LONG)FileTimeToPosixEpoch(&ftUTC));
            return TCL_OK;
        }
#endif
        case OPTION_ZONE: {
            LONG lBias;

            if (GetTimeZoneBias(&lBias)) {
                Tcl_SetLongObj(Tcl_GetObjResult(interp), (LONG)lBias);
            } else {
                Tcl_SetLongObj(Tcl_GetObjResult(interp), 0);
            }

            return TCL_OK;
        }
        default: {
            return TCL_ERROR;
        }
    }
}