/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Windows Volumes

Author:
    neoxed (neoxed@gmail.com) Apr 16, 2005

Abstract:
    Retrieve Windows volume information.

--*/

#include <alcoExt.h>

static BOOL
GetVolumeSize(
    char *volumePath,
    ULONGLONG *bytesFree,
    ULONGLONG *bytesTotal
    );

#define IsDiskSpaceExAvailable()                    \
    (winProcs.getDiskFreeSpaceEx != NULL)

#define IsVolumeMountPointAvailable()               \
    (winProcs.findFirstVolumeMountPoint != NULL &&  \
     winProcs.findNextVolumeMountPoint  != NULL &&  \
     winProcs.findVolumeMountPointClose != NULL &&  \
     winProcs.getVolumeNameForVolumeMountPoint != NULL)


/*++

GetVolumeSize

    Retrieves the total and free space in bytes for a volume.

Arguments:
    volumePath - A pointer to a string that specifies the volume.

    bytesFree  - A pointer to a variable that receives the number of free
                 bytes on the specified volume.

    bytesTotal - A pointer to a variable that receives the total number of
                 bytes on the specified volume.

Return Value:
    If the function succeeds, the return value is non-zero.
    If the function fails, the return value is zero.

--*/
static BOOL
GetVolumeSize(
    char *volumePath,
    ULONGLONG *bytesFree,
    ULONGLONG *bytesTotal
    )
{
    BOOL result;

    assert(volumePath != NULL);
    assert(bytesFree  != NULL);
    assert(bytesTotal != NULL);

    //
    // GetDiskFreeSpaceEx() crashes on NT4, at least it did
    // for me; so we'll use GetDiskFreeSpace() instead.
    //
    if (!IsDiskSpaceExAvailable() || (osVersion.dwPlatformId == VER_PLATFORM_WIN32_NT && osVersion.dwMajorVersion <= 4)) {
        unsigned long bytesPerSector;
        unsigned long freeClusters;
        unsigned long sectorsPerCluster;
        unsigned long totalClusters;

        result = GetDiskFreeSpaceA(volumePath,
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

        result = winProcs.getDiskFreeSpaceEx(volumePath,
            (PULARGE_INTEGER)&freeToCaller,
            (PULARGE_INTEGER)&(*bytesTotal),
            (PULARGE_INTEGER)&(*bytesFree));
    }

    return result;
}

/*++

GetVolumeInfo

    Retrieves information about a volume.

Arguments:
    interp     - Interpreter to use for error reporting.

    volumePath - A pointer to a string that specifies the volume.

    volumeInfo - A pointer to a "VolumeInfo" structure.

Return Value:
    A standard Tcl result.

--*/
int
GetVolumeInfo(
    Tcl_Interp *interp,
    char *volumePath,
    VolumeInfo *volumeInfo
    )
{
    char *type;

    assert(interp     != NULL);
    assert(volumePath != NULL);
    assert(volumeInfo != NULL);

    if (!GetVolumeInformationA(volumePath,
        volumeInfo->name, ARRAYSIZE(volumeInfo->name),
        &volumeInfo->id,
        &volumeInfo->length,
        &volumeInfo->flags,
        NULL, 0)) {

        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "unable to retrieve volume information for \"",
            volumePath, "\": ", TclSetWinError(interp, GetLastError()), NULL);
        return TCL_ERROR;
    }

    if (!GetVolumeSize(volumePath, &volumeInfo->free, &volumeInfo->total)) {
        volumeInfo->free = 0;
        volumeInfo->total = 0;
    }

    switch (GetDriveTypeA(volumePath)) {
        case DRIVE_CDROM:     type = "CD-ROM";    break;
        case DRIVE_FIXED:     type = "Fixed";     break;
        case DRIVE_RAMDISK:   type = "RAM-Disk";  break;
        case DRIVE_REMOTE:    type = "Network";   break;
        case DRIVE_REMOVABLE: type = "Removable"; break;
        default:              type = "Unknown";   break;
    }
    StringCchCopyA(volumeInfo->type, ARRAYSIZE(volumeInfo->type), type);

    return TCL_OK;
}

/*++

GetVolumeList

    Retrieves a list of volumes and mount points.

Arguments:
    interp  - Interpreter to use for error reporting.

    options - OR-ed value of flags that determine the returned volumes.

Return Value:
    A Tcl list object with applicable volumes and mount points. If the
    function fails, NULL is returned and an error message is left in the
    interpreter's result.

--*/
Tcl_Obj *
GetVolumeList(
    Tcl_Interp *interp,
    unsigned short options
    )
{
    HRESULT result;
    char volumeGUID[MAX_PATH];
    char *buffer;
    char *drive;
    size_t bufferLength;
    size_t driveEnd;
    Tcl_Obj *volumeList;
    Tcl_Obj *elementPtr;
    Tcl_Obj *normalPath;

    assert(interp != NULL);

    // Volume mount points were added in Windows 2000.
    if ((options & VOLLIST_FLAG_MOUNTS) &&
            (!IsVolumeMountPointAvailable() ||
             osVersion.dwPlatformId != VER_PLATFORM_WIN32_NT ||
             osVersion.dwMajorVersion < 5)) {
        Tcl_SetResult(interp, "volume mount points are only available on NT5+ systems", TCL_STATIC);
        return NULL;
    }

    volumeList = Tcl_NewObj();

    // Retrieve a list of logical drives.
    bufferLength = GetLogicalDriveStringsA(0, NULL);
    buffer = ckalloc((unsigned int)bufferLength * sizeof(char));
    GetLogicalDriveStringsA((DWORD)bufferLength, buffer);
    drive = buffer;

    result = StringCchLengthA(drive, bufferLength, &driveEnd);
    while (SUCCEEDED(result) && driveEnd) {
        // Append root volume name.
        if (options & VOLLIST_FLAG_LOCAL || options & VOLLIST_FLAG_ROOT) {
            elementPtr = Tcl_NewStringObj(drive, -1);
            Tcl_IncrRefCount(elementPtr);
            normalPath = Tcl_FSGetNormalizedPath(NULL, elementPtr);

            if (normalPath != NULL) {
                Tcl_ListObjAppendElement(NULL, volumeList, normalPath);
            }
            Tcl_DecrRefCount(elementPtr);
        }

        // Append all mount points for the volume.
        if ((options & VOLLIST_FLAG_MOUNTS) && winProcs.getVolumeNameForVolumeMountPoint(drive, volumeGUID, MAX_PATH)) {
            HANDLE mountHandle;
            char mountPath[MAX_PATH];

            mountHandle = winProcs.findFirstVolumeMountPoint(volumeGUID, mountPath, MAX_PATH);
            if (mountHandle != INVALID_HANDLE_VALUE) {
                do {
                    elementPtr = Tcl_NewObj();
                    Tcl_ListObjAppendElement(NULL, elementPtr, Tcl_NewStringObj(drive, -1));
                    Tcl_ListObjAppendElement(NULL, elementPtr, Tcl_NewStringObj(mountPath, -1));

                    // Join the drive and mount target paths.
                    Tcl_IncrRefCount(elementPtr);
                    normalPath = Tcl_FSJoinPath(elementPtr, 2);
                    Tcl_ListObjAppendElement(NULL, volumeList, normalPath);
                    Tcl_DecrRefCount(elementPtr);

                } while (winProcs.findNextVolumeMountPoint(mountHandle, mountPath, MAX_PATH));

                winProcs.findVolumeMountPointClose(mountHandle);
            }
        }

        // Advance to the next drive.
        drive += driveEnd + 1;
        result = StringCchLengthA(drive, bufferLength, &driveEnd);
    }

    ckfree(buffer);
    return volumeList;
}
