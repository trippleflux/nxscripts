#include <nxHelper.h>

__inline BOOL TouchTime(LPCTSTR pszFilePath, LPFILETIME pftTouchTime, WORD wOptions)
{
    BOOL bReturn = FALSE;
    HANDLE hFile = CreateFile(pszFilePath,
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        // Create a handle for a directory or file.
        (wOptions & TOUCH_FLAG_ISDIR) ? FILE_FLAG_BACKUP_SEMANTICS : 0,
        0);

    if (hFile != INVALID_HANDLE_VALUE) {
        bReturn = SetFileTime(hFile,
            (wOptions & TOUCH_FLAG_CTIME) ? pftTouchTime : NULL,
            (wOptions & TOUCH_FLAG_ATIME) ? pftTouchTime : NULL,
            (wOptions & TOUCH_FLAG_MTIME) ? pftTouchTime : NULL);

        CloseHandle(hFile);
    }

    return bReturn;
}

static BOOL RecursiveTouch(LPCTSTR pszCurentPath, LPFILETIME pftTouchTime, WORD wOptions)
{
    TCHAR szPath[MAX_PATH];

    // Touch the directory.
    TouchTime(pszCurentPath, pftTouchTime, wOptions | TOUCH_FLAG_ISDIR);

    if (PathCombine(szPath, pszCurentPath, TEXT("*.*"))) {
        WIN32_FIND_DATA FindData;
        HANDLE hFind = FindFirstFile(szPath, &FindData);

        if (hFind == INVALID_HANDLE_VALUE) {
            return FALSE;
        }

        do {
            if (!_tcscmp(FindData.cFileName, TEXT(".")) ||
                !_tcscmp(FindData.cFileName, TEXT("..")) ||
                !PathCombine(szPath, pszCurentPath, FindData.cFileName)) {
                continue;
            }

            if (FindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
                // Touch the junction, but do not recurse into it (avoid possible infinite loops).
                TouchTime(szPath, pftTouchTime, wOptions | TOUCH_FLAG_ISDIR);

            } else if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // Recurse into the directory.
                RecursiveTouch(szPath, pftTouchTime, wOptions);

            } else if (_tcsnicmp(FindData.cFileName, TEXT(".ioFTPD"), 7) != 0) {
                // Touch the file.
                TouchTime(szPath, pftTouchTime, wOptions);
            }
        } while(FindNextFile(hFind, &FindData));

        FindClose(hFind);
    }

    return TRUE;
}

INT TclTouchCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[])
{
    FILETIME ftTouch;
    BOOL bRetVal;
    CHAR *szPath;
    DWORD dwAttribs;
    INT i;
    ULONG ulClockVal;
    WORD wOptions = 0;

    const static CHAR *szSwitches[] = {
        "-atime", "-ctime", "-mtime", "-recurse", "--", NULL
    };
    enum eSwitches {
        OPTION_ATIME, OPTION_CTIME, OPTION_MTIME, OPTION_RECURSE, OPTION_LAST
    };

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "?switches? path ?clockVal?");
        return TCL_ERROR;
    }

    for (i = 1; i < objc; i++) {
        CHAR *szName;
        INT nIndex;

        szName = Tcl_GetString(objv[i]);
        if (szName[0] != '-') {
            break;
        }

        if (Tcl_GetIndexFromObj(interp, objv[i], szSwitches, "switch", TCL_EXACT, &nIndex) != TCL_OK) {
            return TCL_ERROR;
        }
        switch ((enum eSwitches) nIndex) {
            case OPTION_ATIME: {
                wOptions |= TOUCH_FLAG_ATIME;
                break;
            }
            case OPTION_CTIME: {
                wOptions |= TOUCH_FLAG_MTIME;
                break;
            }
            case OPTION_MTIME: {
                wOptions |= TOUCH_FLAG_CTIME;
                break;
            }
            case OPTION_RECURSE: {
                wOptions |= TOUCH_FLAG_RECURSE;
                break;
            }
            case OPTION_LAST: {
                i++;
                goto endOfForLoop;
            }
            default: {
                return TCL_ERROR; // Should never be reached.
            }
        }
    }

    endOfForLoop:
    objc -= i;
    if (objc < 1 || objc > 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "?switches? path ?clockVal?");
        return TCL_ERROR;
    }

    // Validate the file or directory's existence.
    szPath = Tcl_GetString(objv[i]);
    dwAttribs = GetFileAttributes(szPath);
    if (dwAttribs == INVALID_FILE_ATTRIBUTES) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "unable to touch \"", szPath, "\": no such file or directory", NULL);
        return TCL_ERROR;
    }

    if (objc == 2) {
        if (Tcl_GetLongFromObj(interp, objv[i+1], (ULONG*)&ulClockVal) != TCL_OK) {
            return TCL_ERROR;
        }
        PosixEpochToFileTime((ULONG)ulClockVal, &ftTouch);
    } else {
        GetSystemTimeAsFileTime(&ftTouch);
    }

    // If no file times are specified, then all times are implied.
    if (!(wOptions & TOUCH_FLAG_ATIME) && !(wOptions & TOUCH_FLAG_CTIME) && !(wOptions & TOUCH_FLAG_MTIME)) {
        wOptions |= (TOUCH_FLAG_ATIME | TOUCH_FLAG_CTIME | TOUCH_FLAG_MTIME);
    }

    if ((dwAttribs & FILE_ATTRIBUTE_DIRECTORY) && !(dwAttribs & FILE_ATTRIBUTE_REPARSE_POINT) && (wOptions & TOUCH_FLAG_RECURSE)) {
        bRetVal = RecursiveTouch(szPath, &ftTouch, wOptions);
    } else {
        // Add the ISDIR flag if we're touching a directory non-recusively.
        if (dwAttribs & FILE_ATTRIBUTE_DIRECTORY) {
            wOptions |= TOUCH_FLAG_ISDIR;
        }
        bRetVal = TouchTime(szPath, &ftTouch, wOptions);
    }

    Tcl_SetIntObj(Tcl_GetObjResult(interp), bRetVal);

    return TCL_OK;
}