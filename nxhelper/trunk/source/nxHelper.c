#include <nxHelper.h>

__declspec(dllexport) INT Nxhelper_Init(Tcl_Interp *interp);
__declspec(dllexport) INT Nxhelper_SafeInit(Tcl_Interp *interp);

INT Nxhelper_Init(Tcl_Interp *interp)
{
#ifdef USE_TCL_STUBS
    if (!Tcl_InitStubs(interp, "8.3", 0)) {
        return TCL_ERROR;
    }
#endif

    /* nxHelper Commands:
     *
     * ::nx::decode string
     *  - Decodes a string of data that has been encoded in base64,
     *    returning the decoded version.
     *  - If an internal or system error occurs, an error is thrown.
     *
     * ::nx::encode data
     *  - Encodes "data" in base64, returning the encoded version.
     *  - If an internal or system error occurs, an error is thrown.
     *
     * ::nx::deflate [-level 0-9] data
     *  - Compresses "data" using zLlib's deflate() function.
     *  - If an internal or system error occurs, an error is thrown.
     *
     * ::nx::inflate [-size integer] data
     *  - Decompresses "data" using zLlib's inflate() function.
     *  - If an internal or system error occurs, an error is thrown.
     *
     * ::nx::mp3 file varName
     *  - Retrieves the ID3/MP3 header info for "file" and uses
     *    the array given by "varName" to store information.
     *  - Returns 1 if successful, and 0 otherwise.
     *
     *  - Array Contents:
     *    album     -
     *    artist    -
     *    title     -
     *    track     -
     *    genre     -
     *    comment   -
     *    bitrate   -
     *    frequency -
     *    layer     -
     *    version   -
     *    mode      -
     *    length    -
     *
     * ::nx::time dst
     *  - Returns 1 if daylight savings time is currently in affect, and 0 if not.
     *
     * ::nx::time local (no longer implemented)
     *  - Returns the current local time.
     *
     * ::nx::time utc   (no longer implemented)
     *  - Returns the current time, expressed in UTC.
     *
     * ::nx::time zone
     *  - Returns the bias for local and UTC time translation, expressed in seconds.
     *
     * ::nx::touch ?switches? path ?clockVal?
     *  - If no time attributes are specified, all attributes are set.
     *  - If "clockVal" is not specified, the current time is used.
     *
     *  - Switches:
     *    -atime   = Set file last-access time.
     *    -ctime   = Set file creation time.
     *    -mtime   = Set file modification time.
     *    -recurse = Recursively touch all files and directories.
     *    --       = End of arguments.
     *
     * ::nx::volume drive varName
     *  - Retrieves information for the "drive" and uses
     *    the array given by "varName" to store information.
     *  - Returns 1 if successful, and 0 otherwise.
     -
     *  - Array Contents:
     *    name   - Volume name.
     *    serial - Volume serial number.
     *    free   - Remaining space, expressed in bytes.
     *    total  - Total space, expressed in bytes.
     *    type   - Drive type, the type number corresponds as follows:
     *
     *    #define DRIVE_UNKNOWN     0
     *    #define DRIVE_NO_ROOT_DIR 1
     *    #define DRIVE_REMOVABLE   2
     *    #define DRIVE_FIXED       3
     *    #define DRIVE_REMOTE      4
     *    #define DRIVE_CDROM       5
     *    #define DRIVE_RAMDISK     6
     *
     *    switch -- $volume(type) {
     *        1 {set typeName "Invalid"}
     *        2 {set typeName "Removable"}
     *        3 {set typeName "Fixed"}
     *        4 {set typeName "Network"}
     *        5 {set typeName "CD-ROM"}
     *        6 {set typeName "RAM disk"}
     *        default {set typeName "Unknown"}
     *    }
     */

    Tcl_CreateObjCommand(interp, "::nx::decode",  TclDecodeCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::encode",  TclEncodeCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::deflate", TclDeflateCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::inflate", TclInflateCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::mp3",     TclMp3Cmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::time",    TclTimeCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::touch",   TclTouchCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::volume",  TclVolumeCmd, NULL, NULL);

    if (Tcl_PkgProvide(interp, "nxHelper", STR_VERSION) != TCL_OK) {
        return TCL_ERROR;
    }

    return TCL_OK;
}

INT Nxhelper_SafeInit(Tcl_Interp *interp)
{
    return Nxhelper_Init(interp);
}
