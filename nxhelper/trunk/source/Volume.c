#include <nxHelper.h>

BOOL GetDiskSpace(LPCTSTR pszRootPath, PUINT64 i64FreeBytes, PUINT64 i64TotalBytes)
{
    HMODULE hModKernel;
    OSVERSIONINFO OSVerInfo;
    PFNGETDISKFREESPACEEX pfnGetDiskFreeSpaceEx = NULL;
    BOOL bRetVal;

    // Set pointers to zero (in case the GetDiskFreeSpace or Ex call fails).
    *i64FreeBytes = 0;
    *i64TotalBytes = 0;

    // Retrieve handle to the kernel32 module.
    hModKernel = GetModuleHandle(TEXT("kernel32.dll"));
    if (!hModKernel) {
        hModKernel = LoadLibrary(TEXT("kernel32.dll"));
    }

    // Retrieve the function address of GetDiskFreeSpaceEx(A/W).
#ifdef UNICODE
    pfnGetDiskFreeSpaceEx = (PFNGETDISKFREESPACEEX) GetProcAddress(
        hModKernel, "GetDiskFreeSpaceExW");
#else
    pfnGetDiskFreeSpaceEx = (PFNGETDISKFREESPACEEX) GetProcAddress(
        hModKernel, "GetDiskFreeSpaceExA");
#endif

    // Initialize the OSVERSIONINFO structure.
    OSVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&OSVerInfo);

    // GetDiskFreeSpaceEx crashes on NT4 (at least it did for me),
    // we so we'll use GetDiskFreeSpace instead.
    if (!pfnGetDiskFreeSpaceEx || (OSVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT && OSVerInfo.dwMajorVersion <= 4)) {
        DWORD dwBytesPerSect, dwFreeClusters, dwSectPerClust, dwTotalClusters;

        bRetVal = GetDiskFreeSpace(pszRootPath, &dwSectPerClust, &dwBytesPerSect, &dwFreeClusters, &dwTotalClusters);
        if (bRetVal) {
            *i64TotalBytes = (INT64)dwTotalClusters * dwSectPerClust * dwBytesPerSect;
            *i64FreeBytes = (INT64)dwFreeClusters * dwSectPerClust * dwBytesPerSect;
        }
    } else {
        UINT64 i64FreeToCaller;

        bRetVal = pfnGetDiskFreeSpaceEx(pszRootPath, (PULARGE_INTEGER)&i64FreeToCaller,
            (PULARGE_INTEGER)&(*i64TotalBytes), (PULARGE_INTEGER)&(*i64FreeBytes));
    }

    if (hModKernel) {
        FreeLibrary(hModKernel);
    }

    return bRetVal;
}

INT TclVolumeCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[])
{
    INT nIndex;
    const static CHAR *szOptions[] = {"info", "type", NULL};
    enum eOptions {OPTION_INFO, OPTION_TYPE};

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option ?args?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], szOptions, "option", 0, &nIndex) != TCL_OK) {
        return TCL_ERROR;
    }

    switch ((enum eOptions) nIndex) {
        case OPTION_INFO: {
            UINT64 i64FreeBytes, i64TotalBytes;
            DWORD dwFlags, dwMaxLength, dwSerial;
            TCHAR *pszDrive;
            TCHAR szName[MAX_PATH], szSystem[MAX_PATH];

            if (objc != 4) {
                Tcl_WrongNumArgs(interp, 2, objv, "drive varName");
                return TCL_ERROR;
            }
            pszDrive = Tcl_GetTString(objv[2]);

            if (GetVolumeInformation(pszDrive, szName, MAX_PATH, &dwSerial, &dwMaxLength, &dwFlags,
                    szSystem, MAX_PATH) && GetDiskSpace(pszDrive, &i64FreeBytes, &i64TotalBytes)) {
                Tcl_Obj *pVarObj = Tcl_NewStringObj(Tcl_GetString(objv[3]), -1);
                Tcl_Obj *pFieldObj = Tcl_NewObj();
                Tcl_Obj *pValueObj;

                //
                // GetVolumeInformation information
                //

                // Set varName(flags) to the file system flags.
                Tcl_SetStringObj(pFieldObj, "flags", -1);
                pValueObj = Tcl_NewLongObj((LONG)dwFlags);
                if (!Tcl_ObjSetVar2(interp, pVarObj, pFieldObj, pValueObj, TCL_LEAVE_ERR_MSG)) {
                    return TCL_ERROR;
                }

                // Set varName(name) to the volume name.
                Tcl_SetStringObj(pFieldObj, "name", -1);
                pValueObj = Tcl_NewTStringObj(szName, -1);
                if (!Tcl_ObjSetVar2(interp, pVarObj, pFieldObj, pValueObj, TCL_LEAVE_ERR_MSG)) {
                    return TCL_ERROR;
                }

                // Set varName(serial) to the volume serial number.
                Tcl_SetStringObj(pFieldObj, "serial", -1);
                // Tcl can only represent signed integers, so we'll go one up - wide integers.
                pValueObj = Tcl_NewWideIntObj((Tcl_WideInt)dwSerial);
                if (!Tcl_ObjSetVar2(interp, pVarObj, pFieldObj, pValueObj, TCL_LEAVE_ERR_MSG)) {
                    return TCL_ERROR;
                }

                // Set varName(system) to the file system name.
                Tcl_SetStringObj(pFieldObj, "system", -1);
                pValueObj = Tcl_NewTStringObj(szSystem, -1);
                if (!Tcl_ObjSetVar2(interp, pVarObj, pFieldObj, pValueObj, TCL_LEAVE_ERR_MSG)) {
                    return TCL_ERROR;
                }

                //
                // GetDiskSpace information
                //

                // Set varName(free) to the remaining disk space.
                Tcl_SetStringObj(pFieldObj, "free", -1);
                pValueObj = Tcl_NewWideIntObj((Tcl_WideUInt)i64FreeBytes);
                if (!Tcl_ObjSetVar2(interp, pVarObj, pFieldObj, pValueObj, TCL_LEAVE_ERR_MSG)) {
                    return TCL_ERROR;
                }

                // Set varName(total) to the total disk space.
                Tcl_SetStringObj(pFieldObj, "total", -1);
                pValueObj = Tcl_NewWideIntObj((Tcl_WideUInt)i64TotalBytes);
                if (!Tcl_ObjSetVar2(interp, pVarObj, pFieldObj, pValueObj, TCL_LEAVE_ERR_MSG)) {
                    return TCL_ERROR;
                }

                Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);
            } else {
                Tcl_SetIntObj(Tcl_GetObjResult(interp), 0);
            }

            return TCL_OK;
        }
        case OPTION_TYPE: {
            TCHAR *pszDrive;

            if (objc != 3) {
                Tcl_WrongNumArgs(interp, 2, objv, "drive");
                return TCL_ERROR;
            }

            pszDrive = Tcl_GetTString(objv[2]);
            Tcl_SetIntObj(Tcl_GetObjResult(interp), (INT)GetDriveType(pszDrive));

            return TCL_OK;
        }
        default: {
            return TCL_ERROR; // Should never be reached.
        }
    }
}
