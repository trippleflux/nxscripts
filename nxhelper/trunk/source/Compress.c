#include <nxHelper.h>

INT TclDeflateCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[])
{
    INT  nSize, nLevel, nError;
    CHAR *pcData;
    z_stream sStream;

    if ((objc != 2 && objc != 4) || (objc == 4 && strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-level"))) {
        Tcl_WrongNumArgs(interp, 1, objv, "?-level 0-9? data");
        return TCL_ERROR;
    }

    // Detect the option and extract its value, or use default compression
    if (objc == 4) {
        if (Tcl_GetIntFromObj(interp, objv[2], &nLevel) != TCL_OK || (nLevel < 0 || nLevel > 9)) {

            Tcl_SetStringObj(Tcl_GetObjResult(interp), "invalid compression level: must be between 0 and 9", -1);
            return TCL_ERROR;
        }
    } else {
        nLevel = Z_DEFAULT_COMPRESSION;
    }

    // Uncompressed stream starts at first byte of the last argument
    sStream.next_in = Tcl_GetByteArrayFromObj(objv[objc-1], &(sStream.avail_in));

    // Blow up result object with sufficient bytes...
    nSize = (INT)(sStream.avail_in * 1.01 + 13);
    if (!(pcData = Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), nSize))) {
        goto eOutOfMemory;
    }

    sStream.next_out  = pcData;
    sStream.avail_out = nSize;
    sStream.zalloc = NULL;
    sStream.zfree  = NULL;

    if ((nError = deflateInit(&sStream, nLevel)) != Z_OK) {
        goto eDeflate;
    }

    // Deflate all in one pass. the result must be Z_STREAM_END!. if it just
    // Z_OK, then we didn't compress all of it, because of insufficient memory
    if ((nError = deflate(&sStream, Z_FINISH)) != Z_STREAM_END) {
        nError = (nError == Z_OK) ? Z_BUF_ERROR : nError;
        goto eDeflate;
    }

    if ((nError = deflateEnd(&sStream)) != Z_OK) {
        goto eDeflate;
    }

    // Set result object to actual length of compressed data
    Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), sStream.total_out);
    return TCL_OK;

    eOutOfMemory:
        Tcl_SetStringObj(Tcl_GetObjResult(interp), "unable to deflate data: out of memory", -1);
        return TCL_ERROR;
    eDeflate:
        TclZlibError(interp, "unable to deflate data: ", nError);
        return TCL_ERROR;
}

INT TclInflateCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[])
{
    INT  nDelta, nError, nSize;
    CHAR *pcData;
    z_stream sStream;

    if ((objc != 2 && objc != 4) || (objc == 4 && strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-size"))) {
        Tcl_WrongNumArgs(interp, 1, objv, "?-size integer? data");
        return TCL_ERROR;
    }

    // Detect the -size option and extract its value, or use default size
    if (objc == 4) {
        if (Tcl_GetIntFromObj(interp, objv[2], &nSize) != TCL_OK || nSize < 1) {

            Tcl_SetStringObj(Tcl_GetObjResult(interp), "invalid buffer size: must be a positive integer", -1);
            return TCL_ERROR;
        }
    } else {
        nSize = ZLIB_BUFFER;
    }

    // Compressed stream starts at first byte of the last argument
    sStream.next_in = Tcl_GetByteArrayFromObj(objv[objc-1], &(sStream.avail_in));

    // Blow up result object with some bytes...
    if (!(pcData = Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), nSize))) {
        goto eOutOfMemory;
    }

    sStream.next_out  = pcData;
    sStream.avail_out = nSize;
    sStream.zalloc = NULL;
    sStream.zfree  = NULL;

    if ((nError = inflateInit(&sStream)) != Z_OK) {
        goto eInflate;
    }

    // Additional same as initial memory
    nDelta = nSize;

    while (1) {
        nError = inflate(&sStream, Z_SYNC_FLUSH);

        // True if all is inflated
        if (nError == Z_STREAM_END) {
            break;
        }
        // Z_OK means there's data left, but something else went wrong
        else if (nError != Z_OK) {
            goto eInflate;
        }

        // Increase the object size if we run out of buffer space
        if (sStream.avail_out == 0) {
            if (!(pcData = Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), nSize + nDelta))) {
                goto eOutOfMemory;
            }

            sStream.next_out  = pcData + nSize;
            sStream.avail_out = nDelta;
            nSize += nDelta;
        }
    }

    if ((nError = inflateEnd(&sStream)) != Z_OK) {
        goto eInflate;
    }

    // Set result object to actual length of decompressed data
    Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), sStream.total_out);
    return TCL_OK;

    eOutOfMemory:
        Tcl_SetStringObj(Tcl_GetObjResult(interp), "unable to inflate data: out of memory", -1);
        return TCL_ERROR;
    eInflate:
        TclZlibError(interp, "unable to inflate data: ", nError);
        return TCL_ERROR;
}

static VOID TclZlibError(Tcl_Interp *interp, LPCSTR szMsg, INT nError)
{
    CHAR *pszError;

    switch (nError) {
        case Z_OK           : pszError = "no error"       ; break;
        case Z_STREAM_END   : pszError = "stream end"     ; break;
        case Z_NEED_DICT    : pszError = "need dictionary"; break;
        case Z_ERRNO        : pszError = "see errno"      ; break;
        case Z_STREAM_ERROR : pszError = "stream error"   ; break;
        case Z_DATA_ERROR   : pszError = "data error"     ; break;
        case Z_MEM_ERROR    : pszError = "memory error"   ; break;
        case Z_BUF_ERROR    : pszError = "buffer error"   ; break;
        case Z_VERSION_ERROR: pszError = "version error"  ; break;
        default             : pszError = "unknown error"  ;
    }

    Tcl_ResetResult(interp);
    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp), szMsg, pszError, NULL);
}
