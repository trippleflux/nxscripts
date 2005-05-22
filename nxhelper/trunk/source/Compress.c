#include <nxHelper.h>

static VOID TclZlibError(Tcl_Interp *interp, PCHAR Message, LONG ErrorNum);


INT TclDeflateCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[])
{
    LONG CompressionLevel;
    LONG DestSize;
    LONG ErrorNum;
    PCHAR DestBuffer;
    z_stream ZlibSteam;

    if ((objc != 2 && objc != 4) || (objc == 4 && strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-level"))) {
        Tcl_WrongNumArgs(interp, 1, objv, "?-level 0-9? data");
        return TCL_ERROR;
    }

    // Detect the option and extract its value, or use default compression
    if (objc == 4) {
        if (Tcl_GetIntFromObj(interp, objv[2], &CompressionLevel) != TCL_OK || (CompressionLevel < 0 || CompressionLevel > 9)) {

            Tcl_SetStringObj(Tcl_GetObjResult(interp), "invalid compression level: must be between 0 and 9", -1);
            return TCL_ERROR;
        }
    } else {
        CompressionLevel = Z_DEFAULT_COMPRESSION;
    }

    // Uncompressed stream starts at first byte of the last argument
    ZlibSteam.next_in = Tcl_GetByteArrayFromObj(objv[objc-1], &(ZlibSteam.avail_in));

    // Blow up result object with sufficient bytes...
    DestSize = (INT)(ZlibSteam.avail_in * 1.01 + 13);
    if (!(DestBuffer = Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), DestSize))) {
        goto eOutOfMemory;
    }

    ZlibSteam.next_out  = DestBuffer;
    ZlibSteam.avail_out = DestSize;
    ZlibSteam.zalloc = NULL;
    ZlibSteam.zfree  = NULL;

    if ((ErrorNum = deflateInit(&ZlibSteam, CompressionLevel)) != Z_OK) {
        goto eDeflate;
    }

    // Deflate all in one pass. the result must be Z_STREAM_END!. if it just
    // Z_OK, then we didn't compress all of it, because of insufficient memory
    if ((ErrorNum = deflate(&ZlibSteam, Z_FINISH)) != Z_STREAM_END) {
        ErrorNum = (ErrorNum == Z_OK) ? Z_BUF_ERROR : ErrorNum;
        goto eDeflate;
    }

    if ((ErrorNum = deflateEnd(&ZlibSteam)) != Z_OK) {
        goto eDeflate;
    }

    // Set result object to actual length of compressed data
    Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), ZlibSteam.total_out);
    return TCL_OK;

    eOutOfMemory:
        Tcl_SetStringObj(Tcl_GetObjResult(interp), "unable to deflate data: insufficient memory", -1);
        return TCL_ERROR;
    eDeflate:
        TclZlibError(interp, "unable to deflate data: ", ErrorNum);
        return TCL_ERROR;
}


INT TclInflateCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[])
{
    LONG BufferDelta;
    LONG DestSize;
    LONG ErrorNum;
    PCHAR DestBuffer;
    z_stream ZlibSteam;

    if ((objc != 2 && objc != 4) || (objc == 4 && strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-size"))) {
        Tcl_WrongNumArgs(interp, 1, objv, "?-size integer? data");
        return TCL_ERROR;
    }

    // Detect the -size option and extract its value, or use default size
    if (objc == 4) {
        if (Tcl_GetIntFromObj(interp, objv[2], &DestSize) != TCL_OK || DestSize < 1) {

            Tcl_SetStringObj(Tcl_GetObjResult(interp), "invalid buffer size: must be a positive integer", -1);
            return TCL_ERROR;
        }
    } else {
        DestSize = ZLIB_BUFFER;
    }

    // Compressed stream starts at first byte of the last argument
    ZlibSteam.next_in = Tcl_GetByteArrayFromObj(objv[objc-1], &(ZlibSteam.avail_in));

    // Blow up result object with some bytes...
    if (!(DestBuffer = Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), DestSize))) {
        goto eOutOfMemory;
    }

    ZlibSteam.next_out  = DestBuffer;
    ZlibSteam.avail_out = DestSize;
    ZlibSteam.zalloc = NULL;
    ZlibSteam.zfree  = NULL;

    if ((ErrorNum = inflateInit(&ZlibSteam)) != Z_OK) {
        goto eInflate;
    }

    // Additional same as initial memory
    BufferDelta = DestSize;

    while (1) {
        ErrorNum = inflate(&ZlibSteam, Z_SYNC_FLUSH);

        // True if all is inflated
        if (ErrorNum == Z_STREAM_END) {
            break;
        }
        // Z_OK means there's data left, but something else went wrong
        else if (ErrorNum != Z_OK) {
            goto eInflate;
        }

        // Increase the object size if we run out of buffer space
        if (ZlibSteam.avail_out == 0) {
            if (!(DestBuffer = Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), DestSize + BufferDelta))) {
                goto eOutOfMemory;
            }

            ZlibSteam.next_out  = DestBuffer + DestSize;
            ZlibSteam.avail_out = BufferDelta;
            DestSize += BufferDelta;
        }
    }

    if ((ErrorNum = inflateEnd(&ZlibSteam)) != Z_OK) {
        goto eInflate;
    }

    // Set result object to actual length of decompressed data
    Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), ZlibSteam.total_out);
    return TCL_OK;

    eOutOfMemory:
        Tcl_SetStringObj(Tcl_GetObjResult(interp), "unable to inflate data: insufficient memory", -1);
        return TCL_ERROR;
    eInflate:
        TclZlibError(interp, "unable to inflate data: ", ErrorNum);
        return TCL_ERROR;
}


static VOID TclZlibError(Tcl_Interp *interp, PCHAR Message, LONG ErrorNum)
{
    PCHAR Error;

    switch (ErrorNum) {
        case Z_OK           : Error = "no error"            ; break;
        case Z_STREAM_END   : Error = "stream end"          ; break;
        case Z_NEED_DICT    : Error = "need dictionary"     ; break;
        case Z_ERRNO        : Error = "file error"          ; break;
        case Z_STREAM_ERROR : Error = "stream error"        ; break;
        case Z_DATA_ERROR   : Error = "data error"          ; break;
        case Z_MEM_ERROR    : Error = "insufficient memory" ; break;
        case Z_BUF_ERROR    : Error = "buffer error"        ; break;
        case Z_VERSION_ERROR: Error = "incompatible version"; break;
        default             : Error = "unknown error";
    }

    Tcl_ResetResult(interp);
    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp), Message, Error, NULL);
}
