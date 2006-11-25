/*
 * nxHelper - Tcl extension for nxTools.
 * Copyright (c) 2004-2006 neoxed
 *
 * File Name:
 *   nxTouch.h
 *
 * Author:
 *   neoxed (neoxed@gmail.com) May 22, 2005
 *
 * Abstract:
 *   Implements a method to change file and directory times both
 *   recursively and non-recursively.
 *
 *   Tcl Commands:
 *     ::nx::touch [switches] <path> [clockVal]
 *       - If no time attributes are specified, all attributes are set.
 *       - If "clockVal" is not specified, the current time is used.
 *       - An error is raised if the time cannot be changed on the file or directory.
 *       - Switches:
 *         -atime   = Set file last-access time.
 *         -ctime   = Set file creation time.
 *         -mtime   = Set file modification time.
 *         -recurse = Recursively touch all files and directories.
 *         --       = End of switches.
 */

#include <nxHelper.h>

typedef enum {
    TOUCH_FLAG_ATIME   = 0x00000001,
    TOUCH_FLAG_MTIME   = 0x00000002,
    TOUCH_FLAG_CTIME   = 0x00000004,
    TOUCH_FLAG_ISDIR   = 0x00000008,
    TOUCH_FLAG_RECURSE = 0x00000010
} TOUCH_FLAGS;


static __forceinline DWORD
TouchFile(
    TCHAR *filePath,
    FILETIME *touchTime,
    TOUCH_FLAGS options
    )
{
    HANDLE fileHandle = CreateFile(filePath,
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        /* Create a handle for a directory or file. */
        (options & TOUCH_FLAG_ISDIR) ? FILE_FLAG_BACKUP_SEMANTICS : 0,
        0);

    if (fileHandle == INVALID_HANDLE_VALUE) {
        return GetLastError();
    } else {
        DWORD result = ERROR_SUCCESS;

        if (!SetFileTime(fileHandle,
            (options & TOUCH_FLAG_CTIME) ? touchTime : NULL,
            (options & TOUCH_FLAG_ATIME) ? touchTime : NULL,
            (options & TOUCH_FLAG_MTIME) ? touchTime : NULL)) {

            result = GetLastError();
        }

        CloseHandle(fileHandle);
        return result;
    }
}

static DWORD
RecursiveTouch(
    TCHAR *CurentPath,
    FILETIME *touchTime,
    TOUCH_FLAGS options
    )
{
    DWORD           result = ERROR_SUCCESS;
    HANDLE          findHandle;
    TCHAR           filePath[MAX_PATH];
    WIN32_FIND_DATA findData;

    /* Touch the directory we're entering. */
    TouchFile(CurentPath, touchTime, options | TOUCH_FLAG_ISDIR);

    if (PathCombine(filePath, CurentPath, TEXT("*.*"))) {

        /* Touch contents of the directory. */
        findHandle = FindFirstFile(filePath, &findData);
        if (findHandle == INVALID_HANDLE_VALUE) {
            return GetLastError();
        }

        do {
            if (!_tcscmp(findData.cFileName, TEXT(".")) ||
                !_tcscmp(findData.cFileName, TEXT("..")) ||
                !PathCombine(filePath, CurentPath, findData.cFileName)) {
                continue;
            }

            if (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
                /* Touch the junction, but do not recurse into it (avoid possible infinite loops). */
                TouchFile(filePath, touchTime, options | TOUCH_FLAG_ISDIR);

            } else if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                /* Recurse into the directory. */
                result = RecursiveTouch(filePath, touchTime, options);

            } else if (_tcsnicmp(findData.cFileName, TEXT(".ioFTPD"), 7) != 0) {
                /* Touch the file. */
                TouchFile(filePath, touchTime, options);
            }
        } while(FindNextFile(findHandle, &findData));

        FindClose(findHandle);
    }

    return result;
}

/*
 * TouchObjCmd
 *
 *	 This function provides the "::nx::touch" Tcl command.
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
TouchObjCmd(
    ClientData dummy,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    DWORD           fileAttributes;
    DWORD           result;
    FILETIME        touchTime;
    TCHAR           *filePath;
    TOUCH_FLAGS     options = 0;
    int             i;
    unsigned long   clockVal;
    const static char *switches[] = {
        "-atime", "-ctime", "-mtime", "-recurse", "--", NULL
    };
    enum switchIndices {
        OPTION_ATIME, OPTION_CTIME, OPTION_MTIME, OPTION_RECURSE, OPTION_LAST
    };

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "?switches? path ?clockVal?");
        return TCL_ERROR;
    }

    for (i = 1; i < objc; i++) {
        char *name;
        int index;

        name = Tcl_GetString(objv[i]);
        if (name[0] != '-') {
            break;
        }
        if (Tcl_GetIndexFromObj(interp, objv[i], switches, "switch", TCL_EXACT, &index) != TCL_OK) {
            return TCL_ERROR;
        }

        switch ((enum switchIndices) index) {
            case OPTION_ATIME: {
                options |= TOUCH_FLAG_ATIME;
                break;
            }
            case OPTION_CTIME: {
                options |= TOUCH_FLAG_MTIME;
                break;
            }
            case OPTION_MTIME: {
                options |= TOUCH_FLAG_CTIME;
                break;
            }
            case OPTION_RECURSE: {
                options |= TOUCH_FLAG_RECURSE;
                break;
            }
            case OPTION_LAST: {
                i++;
                goto endOfForLoop;
            }
            default: {
                /* This point is never reached. */
                assert(0);
                return TCL_ERROR;
            }
        }
    }

    endOfForLoop:
    objc -= i;
    if (objc < 1 || objc > 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "?switches? path ?clockVal?");
        return TCL_ERROR;
    }

    /* Validate the file or directory's existence. */
    filePath = Tcl_GetTString(objv[i]);
    fileAttributes = GetFileAttributes(filePath);
    if (fileAttributes == INVALID_FILE_ATTRIBUTES) {
        Tcl_AppendResult(interp, "unable to touch \"", Tcl_GetString(objv[i]),
            "\": no such file or directory", NULL);
        return TCL_ERROR;
    }

    if (objc == 2) {
        if (Tcl_GetLongFromObj(interp, objv[i+1], (unsigned long*)&clockVal) != TCL_OK) {
            return TCL_ERROR;
        }
        PosixEpochToFileTime(clockVal, &touchTime);
    } else {
        GetSystemTimeAsFileTime(&touchTime);
    }

    /* If no file times are specified, then all times are implied. */
    if (!(options & TOUCH_FLAG_ATIME) && !(options & TOUCH_FLAG_CTIME) && !(options & TOUCH_FLAG_MTIME)) {
        options |= (TOUCH_FLAG_ATIME | TOUCH_FLAG_CTIME | TOUCH_FLAG_MTIME);
    }

    if ((fileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !(fileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) && (options & TOUCH_FLAG_RECURSE)) {
        result = RecursiveTouch(filePath, &touchTime, options);
    } else {
        /* OR the TOUCH_FLAG_ISDIR bit if we're touching a directory non-recusively. */
        if (fileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            options |= TOUCH_FLAG_ISDIR;
        }
        result = TouchFile(filePath, &touchTime, options);
    }

    if (result != ERROR_SUCCESS) {
        Tcl_AppendResult(interp, "unable to touch \"", Tcl_GetString(objv[i]), "\": ",
            TclSetWinError(interp, result), NULL);
        return TCL_ERROR;
    }

    return TCL_OK;
}
