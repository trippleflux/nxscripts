#include <nxHelper.h>

BOOL GetDiskSpace(PTCHAR RootPath, PULONGLONG FreeBytes, PUINT64 TotalBytes)
{
    HMODULE ModKernel;
    OSVERSIONINFO OsVerInfo;
    FNGETDISKFREESPACEEX GetDiskFreeSpaceExPtr = NULL;
    BOOL ReturnValue;

    // Set pointers to zero (in case the GetDiskFreeSpace or Ex call fails).
    *FreeBytes = 0;
    *TotalBytes = 0;

    // Retrieve handle to the kernel32 module.
    ModKernel = GetModuleHandle(TEXT("kernel32.dll"));
    if (!ModKernel) {
        ModKernel = LoadLibrary(TEXT("kernel32.dll"));
    }

    // Retrieve the function address of GetDiskFreeSpaceEx(A/W).
#ifdef UNICODE
    GetDiskFreeSpaceExPtr = (FNGETDISKFREESPACEEX) GetProcAddress(
        ModKernel, "GetDiskFreeSpaceExW");
#else
    GetDiskFreeSpaceExPtr = (FNGETDISKFREESPACEEX) GetProcAddress(
        ModKernel, "GetDiskFreeSpaceExA");
#endif

    // Initialize the OSVERSIONINFO structure.
    OsVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&OsVerInfo);

    // GetDiskFreeSpaceEx crashes on NT4 (at least it did for me),
    // we so we'll use GetDiskFreeSpace instead.
    if (!GetDiskFreeSpaceExPtr || (OsVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT && OsVerInfo.dwMajorVersion <= 4)) {
        ULONG BytesPerSector;
        ULONG FreeClusters;
        ULONG SectorsPerCluster;
        ULONG TotalClusters;

        ReturnValue = GetDiskFreeSpace(RootPath, &SectorsPerCluster, &BytesPerSector, &FreeClusters, &TotalClusters);
        if (ReturnValue) {
            *TotalBytes = (INT64)TotalClusters * SectorsPerCluster * BytesPerSector;
            *FreeBytes = (INT64)FreeClusters * SectorsPerCluster * BytesPerSector;
        }
    } else {
        UINT64 FreeToCaller;

        ReturnValue = GetDiskFreeSpaceExPtr(RootPath, (PULARGE_INTEGER)&FreeToCaller,
            (PULARGE_INTEGER)&(*TotalBytes), (PULARGE_INTEGER)&(*FreeBytes));
    }

    if (ModKernel) {
        FreeLibrary(ModKernel);
    }

    return ReturnValue;
}

INT TclVolumeCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[])
{
    INT OptionIndex;
    const static CHAR *Options[] = {"info", "type", NULL};
    enum OptionIndexes {OPTION_INFO, OPTION_TYPE};

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option ?args?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], Options, "option", 0, &OptionIndex) != TCL_OK) {
        return TCL_ERROR;
    }

    switch ((enum OptionIndexes) OptionIndex) {
        case OPTION_INFO: {
            PTCHAR Drive;
            TCHAR VolumeFs[MAX_PATH];
            TCHAR VolumeName[MAX_PATH];
            UINT64 FreeBytes;
            UINT64 TotalBytes;
            ULONG VolumeFlags;
            ULONG VolumeMaxLength;
            ULONG VolumeSerial;

            if (objc != 4) {
                Tcl_WrongNumArgs(interp, 2, objv, "drive varName");
                return TCL_ERROR;
            }
            Drive = Tcl_GetTString(objv[2]);

            if (GetVolumeInformation(Drive, VolumeName, MAX_PATH, &VolumeSerial, &VolumeMaxLength, &VolumeFlags,
                    VolumeFs, MAX_PATH) && GetDiskSpace(Drive, &FreeBytes, &TotalBytes)) {
                Tcl_Obj *VarObj = Tcl_NewStringObj(Tcl_GetString(objv[3]), -1);
                Tcl_Obj *FieldObj = Tcl_NewObj();
                Tcl_Obj *ValueObj;

                //
                // GetVolumeInformation information
                //

                // Set "flags" to the file system flags.
                Tcl_SetStringObj(FieldObj, "flags", -1);
                ValueObj = Tcl_NewLongObj((LONG)VolumeFlags);
                if (!Tcl_ObjSetVar2(interp, VarObj, FieldObj, ValueObj, TCL_LEAVE_ERR_MSG)) {
                    return TCL_ERROR;
                }

                // Set "name" to the volume name.
                Tcl_SetStringObj(FieldObj, "name", -1);
                ValueObj = Tcl_NewTStringObj(VolumeName, -1);
                if (!Tcl_ObjSetVar2(interp, VarObj, FieldObj, ValueObj, TCL_LEAVE_ERR_MSG)) {
                    return TCL_ERROR;
                }

                // Set "serial" to the volume serial number.
                Tcl_SetStringObj(FieldObj, "serial", -1);
                // Tcl can only represent signed integers, so we'll go one up - wide integers.
                ValueObj = Tcl_NewWideIntObj((Tcl_WideInt)VolumeSerial);
                if (!Tcl_ObjSetVar2(interp, VarObj, FieldObj, ValueObj, TCL_LEAVE_ERR_MSG)) {
                    return TCL_ERROR;
                }

                // Set "system" to the file system name.
                Tcl_SetStringObj(FieldObj, "system", -1);
                ValueObj = Tcl_NewTStringObj(VolumeFs, -1);
                if (!Tcl_ObjSetVar2(interp, VarObj, FieldObj, ValueObj, TCL_LEAVE_ERR_MSG)) {
                    return TCL_ERROR;
                }

                //
                // GetDiskSpace information
                //

                // Set "free" to the remaining disk space.
                Tcl_SetStringObj(FieldObj, "free", -1);
                ValueObj = Tcl_NewWideIntObj((Tcl_WideUInt)FreeBytes);
                if (!Tcl_ObjSetVar2(interp, VarObj, FieldObj, ValueObj, TCL_LEAVE_ERR_MSG)) {
                    return TCL_ERROR;
                }

                // Set "total" to the total disk space.
                Tcl_SetStringObj(FieldObj, "total", -1);
                ValueObj = Tcl_NewWideIntObj((Tcl_WideUInt)TotalBytes);
                if (!Tcl_ObjSetVar2(interp, VarObj, FieldObj, ValueObj, TCL_LEAVE_ERR_MSG)) {
                    return TCL_ERROR;
                }

                Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);
            } else {
                Tcl_SetIntObj(Tcl_GetObjResult(interp), 0);
            }

            return TCL_OK;
        }
        case OPTION_TYPE: {
            PTCHAR Drive;

            if (objc != 3) {
                Tcl_WrongNumArgs(interp, 2, objv, "drive");
                return TCL_ERROR;
            }

            Drive = Tcl_GetTString(objv[2]);
            Tcl_SetIntObj(Tcl_GetObjResult(interp), (INT)GetDriveType(Drive));

            return TCL_OK;
        }
        default: {
            return TCL_ERROR; // Should never be reached.
        }
    }
}
