/*
 * nxHelper - Tcl extension for nxTools.
 * Copyright (c) 2005 neoxed
 *
 * File Name:
 *   nxVolume.c
 *
 * Author:
 *   neoxed (neoxed@gmail.com) May 22, 2005
 *
 * Abstract:
 *   Implements a Tcl interface to retrieve Windows volume information.
 *
 *   Tcl Commands:
 *    ::nx::volume info <volume> <varName>
 *      - Retrieves information for the "drive" and uses
 *        the array given by "varName" to store information.
 *      - Array Contents:
 *        name   - Name of the volume.
 *        fs     - File system name.
 *        serial - Volume serial number.
 *        length - Maximum length of a file name.
 *        flags  - Flags associated with the file system.
 *        type   - Volume type, see below for an explanation.
 *        free   - Remaining space, expressed in bytes.
 *        total  - Total space, expressed in bytes.
 *
 *     ::nx::volume type <volume>
 *      - Determines whether a volume is a removable, fixed, CD-ROM, RAM disk,
 *        or network drive.
 */

#include <nxHelper.h>

static BOOL GetVolumeInfo(TCHAR *volumePath, VolumeInfo *volumeInfo);
static BOOL GetVolumeSize(TCHAR *volumePath, ULONGLONG *bytesFree, ULONGLONG *bytesTotal);


/*
 * GetVolumeSize
 *
 *	 Retrieves the total and free space in bytes for a volume.
 *
 * Arguments:
 *   volumePath - A pointer to a string that specifies the volume.
 *   bytesFree  - A pointer to a variable that receives the total number of free
 *                bytes on the specified volume.
 *   bytesTotal - A pointer to a variable that receives the total number of bytes
 *                on the specified volume.
 *
 * Returns:
 *   If the function succeeds, the return value is non-zero.
 *   If the function fails, the return value is zero.
 *
 * Remarks:
 *   None.
 */

static BOOL
GetVolumeSize(TCHAR *volumePath, ULONGLONG *bytesFree, ULONGLONG *bytesTotal)
{
    BOOL result;

    /*
     * GetDiskFreeSpaceEx() crashes on NT4, at least it did
     * for me; so we'll use GetDiskFreeSpace() instead.
     */
    if (GetDiskFreeSpaceExPtr == NULL || (osVersion.dwPlatformId == VER_PLATFORM_WIN32_NT &&
        osVersion.dwMajorVersion <= 4)) {

        ULONG bytesPerSector;
        ULONG freeClusters;
        ULONG sectorsPerCluster;
        ULONG totalClusters;

        result = GetDiskFreeSpace(volumePath,
            &sectorsPerCluster,
            &bytesPerSector,
            &freeClusters,
            &totalClusters);

        if (result) {
            *bytesTotal = (ULONGLONG)totalClusters * sectorsPerCluster * bytesPerSector;
            *bytesFree = (ULONGLONG)freeClusters * sectorsPerCluster * bytesPerSector;
        }
    } else {
        ULONGLONG freeToCaller;

        result = GetDiskFreeSpaceExPtr(volumePath,
            (PULARGE_INTEGER)&freeToCaller,
            (PULARGE_INTEGER)&(*bytesTotal),
            (PULARGE_INTEGER)&(*bytesFree));
    }

    return result;
}


/*
 * GetVolumeInfo
 *
 *	 Retrieves information about a volume.
 *
 * Arguments:
 *   volumePath - A pointer to a string that specifies the volume.
 *   volumeInfo - A pointer to a VolumeInfo structure.
 *
 * Returns:
 *   If the function succeeds, the return value is non-zero.
 *   If the function fails, the return value is zero.
 *
 * Remarks:
 *   None.
 */

static BOOL
GetVolumeInfo(TCHAR *volumePath, VolumeInfo *volumeInfo)
{
    if (!GetVolumeInformation(volumePath,
        volumeInfo->name, VOLUME_NAME_BUFFER,
        &volumeInfo->serial, &volumeInfo->length, &volumeInfo->flags,
        volumeInfo->fs, VOLUME_FS_BUFFER)) {
        return FALSE;
    }

    if (!GetVolumeSize(volumePath, &volumeInfo->bytesFree, &volumeInfo->bytesTotal)) {
        volumeInfo->bytesFree = 0;
        volumeInfo->bytesTotal = 0;
    }

    volumeInfo->type = GetDriveType(volumePath);
    return TRUE;
}


/*
 * VolumeObjCmd
 *
 *	 This function provides the "::nx::volume" Tcl command.
 *
 * Arguments:
 *   dummy  - Not used.
 *   interp - Current interpreter.
 *   objc   - Number of arguments.
 *   objv   - Argument objects.
 *
 * Returns:
 *   A standard Tcl result.
 *
 * Remarks:
 *   None.
 */

int
VolumeObjCmd(ClientData dummy, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    int index;
    const static char *options[] = {"info", "type", NULL};
    enum options {OPTION_INFO, OPTION_TYPE};

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option ?args?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch ((enum options) index) {
        case OPTION_INFO: {
            Tcl_Obj *fieldObj;
            TCHAR *volumePath;
            VolumeInfo volumeInfo;

            if (objc != 4) {
                Tcl_WrongNumArgs(interp, 2, objv, "volume varName");
                return TCL_ERROR;
            }

            volumePath = Tcl_GetTString(objv[2]);
            if (!GetVolumeInfo(volumePath, &volumeInfo)) {
                Tcl_AppendResult(interp, "unable to retrieve volume information: ",
                    TclSetWinError(interp, GetLastError()), NULL);
                return TCL_ERROR;
            }

            /*
             * Tcl_ObjSetVar2() won't create a copy of the field object,
             * so the caller must free the object once finished with it.
             */
            fieldObj = Tcl_NewObj();
            Tcl_IncrRefCount(fieldObj);

            /* Easier then repeating this... */
            #define TCL_STORE_ARRAY(fieldName, valueObj) \
            Tcl_SetStringObj(fieldObj, fieldName, -1); \
            if (Tcl_ObjSetVar2(interp, objv[3], fieldObj, valueObj, TCL_LEAVE_ERR_MSG) == NULL) { \
                Tcl_DecrRefCount(fieldObj); \
                Tcl_DecrRefCount(valueObj); \
                return TCL_ERROR; \
            }

            TCL_STORE_ARRAY("name",   Tcl_NewTStringObj(volumeInfo.name, -1));
            TCL_STORE_ARRAY("fs",     Tcl_NewTStringObj(volumeInfo.fs, -1));
            TCL_STORE_ARRAY("serial", Tcl_NewWideIntObj((Tcl_WideInt)volumeInfo.serial));
            TCL_STORE_ARRAY("length", Tcl_NewLongObj((long)volumeInfo.length));
            TCL_STORE_ARRAY("flags",  Tcl_NewLongObj((long)volumeInfo.flags));
            TCL_STORE_ARRAY("type",   Tcl_NewLongObj((long)volumeInfo.type));
            TCL_STORE_ARRAY("free",   Tcl_NewWideIntObj((Tcl_WideInt)volumeInfo.bytesFree));
            TCL_STORE_ARRAY("total",  Tcl_NewWideIntObj((Tcl_WideInt)volumeInfo.bytesTotal));

            Tcl_DecrRefCount(fieldObj);
            return TCL_OK;
        }
        case OPTION_TYPE: {
            TCHAR *drive;

            if (objc != 3) {
                Tcl_WrongNumArgs(interp, 2, objv, "drive");
                return TCL_ERROR;
            }

            drive = Tcl_GetTString(objv[2]);
            Tcl_SetIntObj(Tcl_GetObjResult(interp), (int)GetDriveType(drive));

            return TCL_OK;
        }
    }

    /* This point should never be reached. */
    Tcl_Panic("unexpected fallthrough");
    return TCL_ERROR;
}
