#include <nxHelper.h>

__inline BOOL TouchFile(PTCHAR FilePath, PFILETIME TouchTime, USHORT Options)
{
    BOOL ReturnValue = FALSE;
    HANDLE FileHandle = CreateFile(FilePath,
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        // Create a handle for a directory or file.
        (Options & TOUCH_FLAG_ISDIR) ? FILE_FLAG_BACKUP_SEMANTICS : 0,
        0);

    if (FileHandle != INVALID_HANDLE_VALUE) {
        ReturnValue = SetFileTime(FileHandle,
            (Options & TOUCH_FLAG_CTIME) ? TouchTime : NULL,
            (Options & TOUCH_FLAG_ATIME) ? TouchTime : NULL,
            (Options & TOUCH_FLAG_MTIME) ? TouchTime : NULL);

        CloseHandle(FileHandle);
    }

    return ReturnValue;
}

static BOOL RecursiveTouch(PTCHAR CurentPath, PFILETIME TouchTime, USHORT Options)
{
    TCHAR FilePath[MAX_PATH];

    // Touch the directory.
    TouchFile(CurentPath, TouchTime, Options | TOUCH_FLAG_ISDIR);

    if (PathCombine(FilePath, CurentPath, TEXT("*.*"))) {
        WIN32_FIND_DATA FindData;
        HANDLE FindHandle = FindFirstFile(FilePath, &FindData);

        if (FindHandle == INVALID_HANDLE_VALUE) {
            return FALSE;
        }

        do {
            if (!_tcscmp(FindData.cFileName, TEXT(".")) ||
                !_tcscmp(FindData.cFileName, TEXT("..")) ||
                !PathCombine(FilePath, CurentPath, FindData.cFileName)) {
                continue;
            }

            if (FindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
                // Touch the junction, but do not recurse into it (avoid possible infinite loops).
                TouchFile(FilePath, TouchTime, Options | TOUCH_FLAG_ISDIR);

            } else if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // Recurse into the directory.
                RecursiveTouch(FilePath, TouchTime, Options);

            } else if (_tcsnicmp(FindData.cFileName, TEXT(".ioFTPD"), 7) != 0) {
                // Touch the file.
                TouchFile(FilePath, TouchTime, Options);
            }
        } while(FindNextFile(FindHandle, &FindData));

        FindClose(FindHandle);
    }

    return TRUE;
}

INT TclTouchCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[])
{
    FILETIME TouchTime;
    BOOL ReturnValue;
    LONG i;
    PTCHAR FilePath;
    ULONG ClockVal;
    ULONG FileAttributes;
    USHORT Options = 0;

    const static CHAR *Switches[] = {
        "-atime", "-ctime", "-mtime", "-recurse", "--", NULL
    };
    enum SwitchIndexes {
        OPTION_ATIME, OPTION_CTIME, OPTION_MTIME, OPTION_RECURSE, OPTION_LAST
    };

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "?switches? path ?clockVal?");
        return TCL_ERROR;
    }

    for (i = 1; i < objc; i++) {
        PCHAR SwitchName;
        INT SwitchIndex;

        SwitchName = Tcl_GetString(objv[i]);
        if (SwitchName[0] != '-') {
            break;
        }

        if (Tcl_GetIndexFromObj(interp, objv[i], Switches, "switch", TCL_EXACT, &SwitchIndex) != TCL_OK) {
            return TCL_ERROR;
        }
        switch ((enum SwitchIndexes) SwitchIndex) {
            case OPTION_ATIME: {
                Options |= TOUCH_FLAG_ATIME;
                break;
            }
            case OPTION_CTIME: {
                Options |= TOUCH_FLAG_MTIME;
                break;
            }
            case OPTION_MTIME: {
                Options |= TOUCH_FLAG_CTIME;
                break;
            }
            case OPTION_RECURSE: {
                Options |= TOUCH_FLAG_RECURSE;
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
    FilePath = Tcl_GetTString(objv[i]);
    FileAttributes = GetFileAttributes(FilePath);
    if (FileAttributes == INVALID_FILE_ATTRIBUTES) {
        Tcl_SetStringObj(Tcl_GetObjResult(interp), "unable to touch item: no such file or directory", -1);
        return TCL_ERROR;
    }

    if (objc == 2) {
        if (Tcl_GetLongFromObj(interp, objv[i+1], (ULONG*)&ClockVal) != TCL_OK) {
            return TCL_ERROR;
        }
        PosixEpochToFileTime((ULONG)ClockVal, &TouchTime);
    } else {
        GetSystemTimeAsFileTime(&TouchTime);
    }

    // If no file times are specified, then all times are implied.
    if (!(Options & TOUCH_FLAG_ATIME) && !(Options & TOUCH_FLAG_CTIME) && !(Options & TOUCH_FLAG_MTIME)) {
        Options |= (TOUCH_FLAG_ATIME | TOUCH_FLAG_CTIME | TOUCH_FLAG_MTIME);
    }

    if ((FileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !(FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) && (Options & TOUCH_FLAG_RECURSE)) {
        ReturnValue = RecursiveTouch(FilePath, &TouchTime, Options);
    } else {
        // Add the ISDIR flag if we're touching a directory non-recusively.
        if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            Options |= TOUCH_FLAG_ISDIR;
        }
        ReturnValue = TouchFile(FilePath, &TouchTime, Options);
    }

    Tcl_SetIntObj(Tcl_GetObjResult(interp), ReturnValue);

    return TCL_OK;
}