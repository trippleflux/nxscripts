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
     *  - Compresses "data" using Zlib's deflate() function.
     *  - If an internal or system error occurs, an error is thrown.
     *
     * ::nx::inflate [-size integer] data
     *  - Decompresses "data" using Zlib's inflate() function.
     *  - If an internal or system error occurs, an error is thrown.
     *
     * ::nx::free drive varName
     *  - Retrieves the free and total space for "drive" and uses
     *    the array given by "varName" to store information.
     *  - Returns 1 if successful, and 0 otherwise.
     *
     * ::nx::mp3 file varName
     *  - Retrieves the ID3/MP3 header info for "file" and uses
     *    the array given by "varName" to store information.
     *  - Returns 1 if successful, and 0 otherwise.
     *
     * ::nx::touch ?switches? path ?clockVal?
     *  - Switches:
     *    -atime   = Set file last-access time.
     *    -ctime   = Set file creation time.
     *    -mtime   = Set file modification time.
     *    -recurse = Recursively touch all files and directories.
     *    -utc     = Use UTC time (or convert clockVal to UTC).
     *    --       = End of arguments.
     *  - If no time attributes are specified, all attributes are set.
     *  - If "clockVal" is not specified, the current time is used.
     *
     * ::nx::time dst
     *  - Returns 1 if daylight savings time is currently in affect, and 0 if not.
     *
     * ::nx::time local
     *  - Returns the current local time.
     *
     * ::nx::time utc
     *  - Returns the current time, expressed in UTC (coordinated universal time).
     *
     * ::nx::time zone
     *  - Returns the bias for local and UTC time translation, expressed in seconds.
     */

    Tcl_CreateObjCommand(interp, "::nx::decode",  TclDecodeCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::encode",  TclEncodeCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::deflate", TclDeflateCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::inflate", TclInflateCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::free",    TclFreeCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::mp3",     TclMp3Cmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::touch",   TclTouchCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::nx::time",    TclTimeCmd, NULL, NULL);

    if (Tcl_PkgProvide(interp, "nxHelper", STR_VERSION) != TCL_OK) {
        return TCL_ERROR;
    }

    return TCL_OK;
}

INT Nxhelper_SafeInit(Tcl_Interp *interp)
{
    return Nxhelper_Init(interp);
}
