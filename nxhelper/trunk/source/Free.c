#include <nxHelper.h>

BOOL GetDiskSpace(LPCTSTR pszRootPath, PUINT64 i64FreeBytes, PUINT64 i64TotalBytes)
{
    HMODULE hModKernel;
    OSVERSIONINFO osVerInfo;
    PFNGETDISKFREESPACEEX pfnGetDiskFreeSpaceEx = NULL;
    BOOL bRetVal;

    // Set pointers to zero (in case the GetDiskFreeSpace call fails).
    *i64FreeBytes = 0;
    *i64TotalBytes = 0;

    // Retrieve handle to the kernel32 module.
    hModKernel = GetModuleHandle(_T("kernel32.dll"));
    if (!hModKernel) {
        hModKernel = LoadLibrary(_T("kernel32.dll"));
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
    osVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osVerInfo);

    // GetDiskFreeSpaceEx crashes on NT4 (at least it did for me),
    // we so we'll use GetDiskFreeSpace instead.
    if (!pfnGetDiskFreeSpaceEx || (osVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT && osVerInfo.dwMajorVersion <= 4)) {
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

INT TclFreeCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[])
{
    UINT64 i64FreeBytes, i64TotalBytes;
    CHAR *pszDrive;

    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 1, objv, "drive varName");
        return TCL_ERROR;
    }
    pszDrive = Tcl_GetString(objv[1]);

    if (GetDiskSpace(pszDrive, &i64FreeBytes, &i64TotalBytes)) {
        Tcl_Obj *pVarObj = Tcl_NewStringObj(Tcl_GetString(objv[2]), -1);
        Tcl_Obj *pFieldObj = Tcl_NewObj();
        Tcl_Obj *pValueObj;

        // Set varName(free) to the remaining disk space.
        Tcl_SetStringObj(pFieldObj, "free", -1);
        pValueObj = Tcl_NewWideIntObj((Tcl_WideUInt) (i64FreeBytes/1024));
        if (!Tcl_ObjSetVar2(interp, pVarObj, pFieldObj, pValueObj, TCL_LEAVE_ERR_MSG)) {
            return TCL_ERROR;
        }

        // Set varName(total) to the total disk space.
        Tcl_SetStringObj(pFieldObj, "total", -1);
        pValueObj = Tcl_NewWideIntObj((Tcl_WideUInt) (i64TotalBytes/1024));
        if (!Tcl_ObjSetVar2(interp, pVarObj, pFieldObj, pValueObj, TCL_LEAVE_ERR_MSG)) {
            return TCL_ERROR;
        }

        Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);
    } else {
        Tcl_SetIntObj(Tcl_GetObjResult(interp), 0);
    }

    return TCL_OK;
}